/*              S I M C O L L I S I O N A L G O . C P P
 * BRL-CAD
 *
 * Copyright (c) 2011 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/*
 * Routines related to performing collision detection using rt.  This
 * is a custom algorithm that replaces the box-box collision algorithm
 * in Bullet.
 */

#include "common.h"

#ifdef HAVE_BULLET

/* system headers */
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btBoxBoxDetector.h>

/* private headers */
#include "./simcollisionalgo.h"


#define USE_PERSISTENT_CONTACTS 1


btRTCollisionAlgorithm::btRTCollisionAlgorithm(btPersistentManifold* mf,
					       const btCollisionAlgorithmConstructionInfo& ci,
					       btCollisionObject* obj0,
					       btCollisionObject* obj1)
    : btActivatingCollisionAlgorithm(ci, obj0, obj1),
      m_ownManifold(false),
      m_manifoldPtr(mf)
{
    if (!m_manifoldPtr && m_dispatcher->needsCollision(obj0, obj1)) {
	m_manifoldPtr = m_dispatcher->getNewManifold(obj0, obj1);
	m_ownManifold = true;
    }
}


btRTCollisionAlgorithm::~btRTCollisionAlgorithm()
{
    if (m_ownManifold) {
	if (m_manifoldPtr)
	    m_dispatcher->releaseManifold(m_manifoldPtr);
    }
}


void
btRTCollisionAlgorithm::processCollision(btCollisionObject* body0,
					 btCollisionObject* body1,
					 const btDispatcherInfo& dispatchInfo,
					 btManifoldResult* resultOut)
{
    if (!m_manifoldPtr)
	return;

    btCollisionObject* col0 = body0;
    btCollisionObject* col1 = body1;
    btBoxShape* box0 = (btBoxShape*)col0->getCollisionShape();
    btBoxShape* box1 = (btBoxShape*)col1->getCollisionShape();

    //quellage
    bu_log("%d", dispatchInfo.m_stepCount);

    /// report a contact. internally this will be kept persistent, and contact reduction is done
    resultOut->setPersistentManifold(m_manifoldPtr);
#ifndef USE_PERSISTENT_CONTACTS
    m_manifoldPtr->clearManifold();
#endif //USE_PERSISTENT_CONTACTS

    btDiscreteCollisionDetectorInterface::ClosestPointInput input;
    input.m_maximumDistanceSquared = BT_LARGE_FLOAT;
    input.m_transformA = body0->getWorldTransform();
    input.m_transformB = body1->getWorldTransform();

    //This part will get replaced with a call to rt
   //btBoxBoxDetector detector(box0, box1);
   //detector.getClosestPoints(input, *resultOut, dispatchInfo.m_debugDraw);


    //------------------- DEBUG ---------------------------

    int i;
    int num_contacts;
    struct sim_manifold *current_manifold;

    //Get the user pointers to struct rigid_body, for printing the body name
    struct rigid_body *rbA = (struct rigid_body *)col0->getUserPointer();
    struct rigid_body *rbB = (struct rigid_body *)col1->getUserPointer();

    if (rbA != NULL && rbB != NULL) {

		btPersistentManifold* contactManifold = resultOut->getPersistentManifold();

		current_manifold =
			(struct sim_manifold *)bu_malloc(sizeof(struct sim_manifold), "sim_manifold: current_manifold");
		current_manifold->next = NULL;
		current_manifold->rbA = rbA;
		current_manifold->rbB = rbB;

		//Add manifold to rbB
		if (rbB->head_manifold == NULL) {
			rbB->head_manifold = current_manifold;
		} else{
			//Go upto the last manifold, keeping a ptr 1 node behind
			struct sim_manifold *p1 = rbB->head_manifold, *p2;
			while (p1 != NULL) {
			p2 = p1;
			p1 = p1->next;
			}

			p2->next = current_manifold;
			//print_manifold_list(rb->head_manifold);
		}
		rbB->num_manifolds++;

		bu_log("processCollision(box/box): %s & %s \n", rbA->rb_namep, rbB->rb_namep);

		//Get the number of points in this manifold
		num_contacts = contactManifold->getNumContacts();
		current_manifold->num_bt_contacts = num_contacts;


		bu_log("processCollision : Manifold contacts : %d\n", num_contacts);

		//Iterate over the points for this manifold
		for (i=0; i<num_contacts; i++) {
			btManifoldPoint& pt = contactManifold->getContactPoint(i);

			btVector3 ptA = pt.getPositionWorldOnA();
			btVector3 ptB = pt.getPositionWorldOnB();

			VMOVE(current_manifold->bt_contacts[i].ptA, ptA);
			VMOVE(current_manifold->bt_contacts[i].ptB, ptB);
			VMOVE(current_manifold->bt_contacts[i].normalWorldOnB, pt.m_normalWorldOnB);

			bu_log("%d, %s(%f, %f, %f) , %s(%f, %f, %f), n(%f, %f, %f)\n",
			   i+1,
			   rbA->rb_namep, ptA[0], ptA[1], ptA[2],
			   rbB->rb_namep, ptB[0], ptB[1], ptB[2],
			   pt.m_normalWorldOnB[0], pt.m_normalWorldOnB[1], pt.m_normalWorldOnB[2]);

		}
    }


   //Scan all manifolds of rbB looking for a rbA-rbB manifold
   for (current_manifold = rbB->head_manifold; current_manifold != NULL;
   	     current_manifold = current_manifold->next) {

	   //Find the manifold in rbB's list which connects rbB and rbA
	   if( bu_strcmp(current_manifold->rbA->rb_namep, rbA->rb_namep) &&
			   bu_strcmp(current_manifold->rbB->rb_namep, rbB->rb_namep) ){

		   // Now add the rt contact pairs
		   for (i=0; i<current_manifold->num_rt_contacts; i++){

			   btVector3 ptB, normalWorldOnB;
			   VMOVE(ptB, current_manifold->rt_contacts[i].ptB);
			   VMOVE(normalWorldOnB, current_manifold->rt_contacts[i].normalWorldOnB);

			   //Negative depth for penetration
			   resultOut->addContactPoint(ptB, normalWorldOnB,
					   -(current_manifold->rt_contacts[i].depth));
		   }

	   }

   }


    //------------------------------------------------------

#ifdef USE_PERSISTENT_CONTACTS
    //  refreshContactPoints is only necessary when using persistent contact points. otherwise all points are newly added
    if (m_ownManifold) {
	resultOut->refreshContactPoints();
    }
#endif //USE_PERSISTENT_CONTACTS

}


btScalar
btRTCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* /*body0*/, btCollisionObject* /*body1*/, const btDispatcherInfo& /*dispatchInfo*/, btManifoldResult* /*resultOut*/)
{
    //not yet
    return 1.f;
}


#endif

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
