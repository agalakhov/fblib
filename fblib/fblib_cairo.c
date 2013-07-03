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

#include "fblib.h"

#include <stddef.h>

#include <cairo.h>

cairo_surface_t* fb_cairo_surface_create(const struct fb_visual* vis) {
    cairo_format_t fmt = CAIRO_FORMAT_INVALID;
    switch (vis->format) {
    case FB_RGB888:
        fmt = CAIRO_FORMAT_RGB24;
        break;
    case FB_RGB565:
        fmt = CAIRO_FORMAT_RGB16_565;
        break;
    case FB_INVALID:
    default:
        return NULL;
    }
    return cairo_image_surface_create_for_data((unsigned char*)vis->data, fmt,
                                               vis->width, vis->height,
                                               fb_get_pixel_size(vis->format) * vis->stride);
}
