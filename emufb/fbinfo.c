/*
 * Copyright (C) 2011-2013 Alexey Galakhov <agalakhov@gmail.com>
 *
 * Licensed under the GNU General Public License Version 3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "fbinfo.h"

#include "common.h"

#include "fblib_simfb.h"

#include <stddef.h>
#include <math.h>
#include <string.h>

void fill_fbinfo(struct fblib_sim_shm* shmem, size_t shmsize,
                 unsigned width, unsigned height, float diagonal,
                 enum m_format mode) {
    const unsigned pixsize = mode2size(mode);
    const float pixmm = diagonal * 25.4 / sqrtf((float)width * width + (float)height * height);

    memset(&shmem->fix, 0, sizeof(shmem->fix));
    strncpy(shmem->fix.id, "Simulated FB", sizeof(shmem->fix.id));
    shmem->fix.smem_start = sizeof(struct fblib_sim_shm);
    shmem->fix.smem_len = shmsize;
    shmem->fix.type = FB_TYPE_PACKED_PIXELS;
    shmem->fix.visual = FB_VISUAL_DIRECTCOLOR;
    shmem->fix.line_length = width * pixsize;

    memset(&shmem->var, 0, sizeof(shmem->var));
    shmem->var.xres = shmem->var.xres_virtual = width;
    shmem->var.yres = shmem->var.yres_virtual = height;
    shmem->var.bits_per_pixel = 8 * pixsize;
    switch (mode) {
    case M_RGB888:
        shmem->var.red.offset   = 16;
        shmem->var.green.offset =  8;
        shmem->var.blue.offset  =  0;
        shmem->var.red.length = shmem->var.green.length = shmem->var.blue.length = 8;
        break;
    case M_RGB565:
        shmem->var.red.offset   = 11;
        shmem->var.green.offset =  5;
        shmem->var.blue.offset  =  0;
        shmem->var.red.length = shmem->var.blue.length = 5;
        shmem->var.green.length = 6;
        break;
    }
    shmem->var.width  = (uint32_t)round((float)width  * pixmm);
    shmem->var.height = (uint32_t)round((float)height * pixmm);
    M_MODE(shmem->var.reserved) = mode;
    M_DIAG(shmem->var.reserved) = (uint32_t)round(diagonal * 100);
}
