/*
 *			G _ S P H . C
 *
 *  Purpose -
 *	Intersect a ray with a Sphere
 *	Special case of the Generalized Ellipsoid
 *
 *  Authors -
 *	Phillip Dykstra
 *	Dave Becker		(Vectorization)
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSsph[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "./debug.h"

/*
 *  Algorithm:
 *  
 *  Given V, A, where |A| = Radius, there is a set of points on this sphere
 *  
 *  { (x,y,z) | (x,y,z) is on sphere defined by V, A }
 *  
 *  To find the intersection of a line with the sphere, consider
 *  the parametric line L:
 *  
 *  	L : { P(n) | P + t(n) . D }
 *  
 *  Call W the actual point of intersection between L and the sphere.
 *
 *  NORMALS.  Given the point W on the sphere, what is the vector
 *  normal to the tangent plane at that point?
 *  
 *  N = (W - V) / |A|
 */

struct sph_specific {
	vect_t	sph_V;		/* Vector to center of sphere */
	fastf_t	sph_radsq;	/* Radius squared */
	fastf_t	sph_invrad;	/* Inverse radius (for normal) */
	fastf_t	sph_rad;	/* Radius */
	mat_t	sph_SoR;	/* Rotate and scale for UV mapping */
};

/* Should be in a header file to share betwee g_ell.c and g_sph.c */
struct ell_internal  {
	point_t	v;
	vect_t	a;
	vect_t	b;
	vect_t	c;
};

/*
 *  			R T _ S P H _ P R E P
 *  
 *  Given a pointer to a GED database record, and a transformation matrix,
 *  determine if this is a valid sphere, and if so, precompute various
 *  terms of the formula.
 *  
 *  Returns -
 *  	0	SPH is OK
 *  	!0	Error in description
 *  
 *  Implicit return -
 *  	A struct sph_specific is created, and it's address is stored in
 *  	stp->st_specific for use by rt_sph_shot().
 *	If the ELL is really a SPH, stp->st_id is modified to ID_SPH.
 */
int
rt_sph_prep( stp, rec, rtip )
struct soltab		*stp;
struct rt_i		*rtip;
union record		*rec;
{
	register struct sph_specific *sph;
	LOCAL fastf_t	magsq_a, magsq_b, magsq_c;
	LOCAL vect_t	Au, Bu, Cu;	/* A,B,C with unit length */
	LOCAL fastf_t	f;
	struct ell_internal ei;
	int		i;

	if( rec == (union record *)0 )  {
		rec = db_getmrec( rtip->rti_dbip, stp->st_dp );
		i = rt_ell_import( &ei, rec, stp->st_pathmat );
		rt_free( (char *)rec, "ell record" );
	} else {
		i = rt_ell_import( &ei, rec, stp->st_pathmat );
	}
	if( i < 0 )  {
		rt_log("rt_sph_setup(%s): db import failure\n", stp->st_name);
		return(-1);		/* BAD */
	}

	/* Validate that |A| > 0, |B| > 0, |C| > 0 */
	magsq_a = MAGSQ( ei.a );
	magsq_b = MAGSQ( ei.b );
	magsq_c = MAGSQ( ei.c );
	if( magsq_a < 0.005 || magsq_b < 0.005 || magsq_c < 0.005 ) {
		rt_log("sph(%s):  zero length A, B, or C vector\n",
			stp->st_name );
		return(1);		/* BAD */
	}

	/* Validate that |A|, |B|, and |C| are nearly equal */
	if( fabs(magsq_a - magsq_b) > 0.0001
	    || fabs(magsq_a - magsq_c) > 0.0001 ) {
#if 0	    	
	    	/* Ordinarily, don't say anything here, will handle as ELL */
		rt_log("sph(%s):  non-equal length A, B, C vectors\n",
			stp->st_name );
#endif
		return(1);		/* ELL, not SPH */
	}

	/* Create unit length versions of A,B,C */
	f = 1.0/sqrt(magsq_a);
	VSCALE( Au, ei.a, f );
	f = 1.0/sqrt(magsq_b);
	VSCALE( Bu, ei.b, f );
	f = 1.0/sqrt(magsq_c);
	VSCALE( Cu, ei.c, f );

	/* Validate that A.B == 0, B.C == 0, A.C == 0 (check dir only) */
	f = VDOT( Au, Bu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("sph(%s):  A not perpendicular to B, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( Bu, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("sph(%s):  B not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}
	f = VDOT( Au, Cu );
	if( ! NEAR_ZERO(f, 0.005) )  {
		rt_log("sph(%s):  A not perpendicular to C, f=%f\n",stp->st_name, f);
		return(1);		/* BAD */
	}

	/*
	 *  This ELL is really an SPH
	 */
	stp->st_id = ID_SPH;		/* "fix" soltab ID */

	/* Solid is OK, compute constant terms now */
	GETSTRUCT( sph, sph_specific );
	stp->st_specific = (genptr_t)sph;

	VMOVE( sph->sph_V, ei.v );

	sph->sph_radsq = magsq_a;
	sph->sph_rad = sqrt(sph->sph_radsq);
	sph->sph_invrad = 1.0 / sph->sph_rad;

	/*
	 * Save the matrix which rotates our ABC to world
	 * XYZ respectively, and scales points on surface
	 * to unit length.  Used here in UV mapping.
	 * See ell.c for details.
	 */
	mat_idn( sph->sph_SoR );
	VSCALE( &sph->sph_SoR[0], ei.a, 1.0/magsq_a );
	VSCALE( &sph->sph_SoR[4], ei.b, 1.0/magsq_b );
	VSCALE( &sph->sph_SoR[8], ei.c, 1.0/magsq_c );

	/* Compute bounding sphere */
	VMOVE( stp->st_center, sph->sph_V );
	stp->st_aradius = stp->st_bradius = sph->sph_rad;

	/* Compute bounding RPP */
	stp->st_min[X] = sph->sph_V[X] - sph->sph_rad;
	stp->st_max[X] = sph->sph_V[X] + sph->sph_rad;
	stp->st_min[Y] = sph->sph_V[Y] - sph->sph_rad;
	stp->st_max[Y] = sph->sph_V[Y] + sph->sph_rad;
	stp->st_min[Z] = sph->sph_V[Z] - sph->sph_rad;
	stp->st_max[Z] = sph->sph_V[Z] + sph->sph_rad;

	return(0);			/* OK */
}

/*
 *			R T _ S P H _ P R I N T
 */
void
rt_sph_print( stp )
register struct soltab *stp;
{
	register struct sph_specific *sph =
		(struct sph_specific *)stp->st_specific;

	VPRINT("V", sph->sph_V);
	rt_log("Rad %g\n", sph->sph_rad);
	rt_log("Radsq %g\n", sph->sph_radsq);
	rt_log("Invrad %g\n", sph->sph_invrad);
	mat_print("S o R", sph->sph_SoR );
}

/*
 *  			R T _ S P H _ S H O T
 *  
 *  Intersect a ray with a sphere.
 *  If an intersection occurs, a struct seg will be acquired
 *  and filled in.
 *  Notes: In the quadratic equation,
 *   A is MAGSQ(r_dir) which is always equal to 1, so it does not appear.
 *   The sign of B is reversed (vector is reversed) to save negation.
 *   We have factored out the 2 and 4 constants.
 *  Claim: The straight quadratic formula leads to precision problems
 *   if either A or C are small.  In our case A is always 1.  C is a
 *   radial distance of the ray origin from the sphere surface.  Thus
 *   if we are shooting from near the surface we may have problems.
 *   XXX - investigate this.
 *  
 *  Returns -
 *  	0	MISS
 *	>0	HIT
 */
int
rt_sph_shot( stp, rp, ap, seghead )
struct soltab		*stp;
register struct xray	*rp;
struct application	*ap;
struct seg		*seghead;
{
	register struct sph_specific *sph =
		(struct sph_specific *)stp->st_specific;
	register struct seg *segp;
	LOCAL vect_t	ov;		/* ray orgin to center (V - P) */
	FAST fastf_t	magsq_ov;	/* length squared of ov */
	FAST fastf_t	b;		/* second term of quadratic eqn */
	FAST fastf_t	root;		/* root of radical */

	VSUB2( ov, sph->sph_V, rp->r_pt );
	b = VDOT( rp->r_dir, ov );
	magsq_ov = MAGSQ(ov);

	if( magsq_ov >= sph->sph_radsq ) {
		/* ray origin is outside of sphere */
		if( b < 0 ) {
			/* ray direction is away from sphere */
			return(0);		/* No hit */
		}
		root = b*b - magsq_ov + sph->sph_radsq;
		if( root <= 0 ) {
			/* no real roots */
			return(0);		/* No hit */
		}
	} else {
		root = b*b - magsq_ov + sph->sph_radsq;
	}
	root = sqrt(root);

	RT_GET_SEG(segp, ap->a_resource);
	segp->seg_stp = stp;

	/* we know root is positive, so we know the smaller t */
	segp->seg_in.hit_dist = b - root;
	segp->seg_out.hit_dist = b + root;
	RT_LIST_INSERT( &(seghead->l), &(segp->l) );
	return(2);			/* HIT */
}

#define SEG_MISS(SEG)		(SEG).seg_stp=(struct soltab *) 0;	
/*
 *			R T _ S P H _ V S H O T
 *
 *  This is the Becker vectorized version
 */
void
rt_sph_vshot( stp, rp, segp, n, resp)
struct soltab	       *stp[]; /* An array of solid pointers */
struct xray		*rp[]; /* An array of ray pointers */
struct  seg            segp[]; /* array of segs (results returned) */
int		  	    n; /* Number of ray/object pairs */
struct resource         *resp; /* pointer to a list of free segs */
{
	register struct sph_specific *sph;
	LOCAL vect_t	ov;		/* ray orgin to center (V - P) */
	FAST fastf_t	magsq_ov;	/* length squared of ov */
	FAST fastf_t	b;		/* second term of quadratic eqn */
	FAST fastf_t	root;		/* root of radical */
	register int    i;

	/* for each ray/sphere pair */
#	include "noalias.h"
	for(i = 0; i < n; i++){
#if !CRAY	/* XXX currently prevents vectorization on cray */
	 	if (stp[i] == 0) continue; /* stp[i] == 0 signals skip ray */
#endif

		sph = (struct sph_specific *)stp[i]->st_specific;
		VSUB2( ov, sph->sph_V, rp[i]->r_pt );
		b = VDOT( rp[i]->r_dir, ov );
	        magsq_ov = MAGSQ(ov);

		if( magsq_ov >= sph->sph_radsq ) {
			/* ray origin is outside of sphere */
			if( b < 0 ) {
				/* ray direction is away from sphere */
				SEG_MISS(segp[i]);		/* No hit */
				continue;
			}
			root = b*b - magsq_ov + sph->sph_radsq;
			if( root <= 0 ) {
				/* no real roots */
				SEG_MISS(segp[i]);		/* No hit */
				continue;
			}
		} else {
			root = b*b - magsq_ov + sph->sph_radsq;
		}
		root = sqrt(root);

		segp[i].seg_stp = stp[i];

		/* we know root is positive, so we know the smaller t */
		segp[i].seg_in.hit_dist = b - root;
		segp[i].seg_out.hit_dist = b + root;
	}
}

/*
 *  			R T _ S P H _ N O R M
 *  
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_sph_norm( hitp, stp, rp )
register struct hit *hitp;
struct soltab *stp;
register struct xray *rp;
{
	register struct sph_specific *sph =
		(struct sph_specific *)stp->st_specific;

	VJOIN1( hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir );
	VSUB2( hitp->hit_normal, hitp->hit_point, sph->sph_V );
	VSCALE( hitp->hit_normal, hitp->hit_normal, sph->sph_invrad );
}

/*
 *			R T _ S P H _ C U R V E
 *
 *  Return the curvature of the sphere.
 */
void
rt_sph_curve( cvp, hitp, stp )
register struct curvature *cvp;
register struct hit *hitp;
struct soltab *stp;
{
	register struct sph_specific *sph =
		(struct sph_specific *)stp->st_specific;

 	cvp->crv_c1 = cvp->crv_c2 = - sph->sph_invrad;

	/* any tangent direction */
 	vec_ortho( cvp->crv_pdir, hitp->hit_normal );
}

/*
 *  			R T _ S P H _ U V
 *  
 *  For a hit on the surface of an SPH, return the (u,v) coordinates
 *  of the hit point, 0 <= u,v <= 1.
 *  u = azimuth
 *  v = elevation
 */
void
rt_sph_uv( ap, stp, hitp, uvp )
struct application *ap;
struct soltab *stp;
register struct hit *hitp;
register struct uvcoord *uvp;
{
	register struct sph_specific *sph =
		(struct sph_specific *)stp->st_specific;
	LOCAL fastf_t r;
	LOCAL vect_t work;
	LOCAL vect_t pprime;

	/* hit_point is on surface;  project back to unit sphere,
	 * creating a vector from vertex to hit point which always
	 * has length=1.0
	 */
	VSUB2( work, hitp->hit_point, sph->sph_V );
	MAT4X3VEC( pprime, sph->sph_SoR, work );
	/* Assert that pprime has unit length */

	/* U is azimuth, atan() range: -pi to +pi */
	uvp->uv_u = mat_atan2( pprime[Y], pprime[X] ) * rt_inv2pi;
	if( uvp->uv_u < 0 )
		uvp->uv_u += 1.0;
	/*
	 *  V is elevation, atan() range: -pi/2 to +pi/2,
	 *  because sqrt() ensures that X parameter is always >0
	 */
	uvp->uv_v = mat_atan2( pprime[Z],
		sqrt( pprime[X] * pprime[X] + pprime[Y] * pprime[Y]) ) *
		rt_invpi + 0.5;

	/* approximation: r / (circumference, 2 * pi * aradius) */
	r = ap->a_rbeam + ap->a_diverge * hitp->hit_dist;
	uvp->uv_du = uvp->uv_dv =
		rt_inv2pi * r / stp->st_aradius;
}

/*
 *		R T _ S P H _ F R E E
 */
void
rt_sph_free( stp )
register struct soltab *stp;
{
	register struct sph_specific *sph =
		(struct sph_specific *)stp->st_specific;

	rt_free( (char *)sph, "sph_specific" );
}

int
rt_sph_class()
{
	return(0);
}

/* ELL versions of the plot and tess functions are used */
