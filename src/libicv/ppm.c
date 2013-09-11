/*                           P P M . C
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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
/** @file ppm.c
 *
 * This file contains reading and writting routines for ppm format.
 *
 */
 
#include "common.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>	/* for file mode info in WRMODE */
#include <fcntl.h>

#include "bio.h"
#include "vmath.h"
#include "bu.h"
#include "bn.h"
#include "icv.h"

#define WRMODE S_IRUSR|S_IRGRP|S_IROTH

/* defined in encoding.c */
extern HIDDEN unsigned char *data2uchar(const icv_image_t *bif);

/* flip an image vertically */
HIDDEN int
image_flip(unsigned char *buf, int width, int height)
{
    unsigned char *buf2;
    int i;
    size_t pitch = width * 3 * sizeof(char);

    buf2 = (unsigned char *)bu_malloc((size_t)(height * pitch), "image flip");
    for (i=0 ; i<height ; i++)
	memcpy(buf2+i*pitch, buf+(height-i)*pitch, pitch);
    memcpy(buf, buf2, height * pitch);
    bu_free(buf2, "image flip");
    return 0;
}

HIDDEN int
ppm_write(icv_image_t *bif, const char *filename)
{
    unsigned char *data;
    int fd;
    size_t ret, size;

    /* FIXME: should not be introducing fixed size buffers */
    char buf[BUFSIZ] = {0};

    if (bif->color_space == ICV_COLOR_SPACE_GRAY) {
	icv_gray2rgb(bif);
    } else if (bif->color_space != ICV_COLOR_SPACE_RGB) {
	bu_log("ppm_write : Color Space conflict");
	return -1;
    }
    data =  data2uchar(bif);
    size = (size_t) bif->width*bif->height*3;
    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, WRMODE);
    image_flip(data, bif->width, bif->height);
    snprintf(buf, BUFSIZ, "P6 %d %d 255\n", bif->width, bif->height);
    ret = write(fd, buf, strlen(buf));
    ret = write(fd, data, size);
    close(fd);
    if (ret != size) {
	bu_log("ppm_write : Short Write");
	return -1;
    }
    return 0;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
