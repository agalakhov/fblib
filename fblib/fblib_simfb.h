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

#ifndef _FBLIB_SIMFB_H_
#define _FBLIB_SIMFB_H_

#include <stdint.h>

#include <linux/fb.h>

#define FBLIB_SIMENV "FBLIB_SIMULATE"
#define FBLIB_SHM "/fblib_devfb-"

struct fblib_sim_shm {
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    uint8_t                  buf[0] __attribute__((aligned(32)));
} __attribute__((aligned(32)));

#endif /* _FBLIB_SIMFB_H_ */
