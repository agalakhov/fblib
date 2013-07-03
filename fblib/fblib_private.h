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

#ifndef _FBLIB_PRIVATE_H_
#define _FBLIB_PRIVATE_H_

#include "fblib.h"

#include <stddef.h>
#include <stdint.h>
#include <linux/fb.h>

struct fbdev {
    uint8_t*                    fb;
    void*                       fb_base;
    enum fb_format              format;
    int                         fd;
    int                         fd_con;
    int                         last_vt;
    struct fb_fix_screeninfo    fix;
    struct fb_var_screeninfo    var;
};

const char* fbp_getenv(const char** vars, const char* def, const char* over);

#endif /* _FBLIB_PRIVATE_H_ */
