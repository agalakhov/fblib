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
#include "fblib_private.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct fb_visual_impl {
    volatile unsigned refs;
    unsigned          x_origin;
    unsigned          y_origin;
    unsigned          linelen;
    uint8_t*          parent;
    uint8_t           buf[0];
};

static struct fb_visual_impl* fbp_visual_new(const struct fb_visual* src) {
    unsigned ps = fb_get_pixel_size(src->format);
    size_t bufsz = src->width * src->height * ps;
    struct fb_visual_impl* impl = calloc(1, sizeof(struct fb_visual_impl) + bufsz);
    impl->refs = 1;
    impl->x_origin = src->x_origin;
    impl->y_origin = src->y_origin;
    impl->linelen  = src->stride * ps;
    impl->parent   = src->data;
    return impl;
}

static struct fb_visual_impl* fbp_visual_ref(struct fb_visual_impl* impl) {
    if (! impl) return impl;
    impl->refs++; /* TODO ATOMIC */
    return impl;
}

static void fbp_visual_unref(struct fb_visual_impl* impl) {
    if (! impl) return;
    if (0 == impl->refs--) /* TODO ATOMIC */
        free(impl);
}

void fb_commit_visual(struct fb_visual* vis) {
    unsigned y;
    unsigned ps = fb_get_pixel_size(vis->format);
    struct fb_visual_impl* impl = vis->_private.impl;
    uint8_t* src = vis->data;
    uint8_t* dst;
    unsigned linelen = vis->stride * ps;
    if (! impl) return;
    assert(vis->x_origin >= impl->x_origin);
    assert(vis->y_origin >= impl->y_origin);
    dst = impl->parent
        + (vis->x_origin - impl->x_origin) * ps
        + (vis->y_origin - impl->y_origin) * impl->linelen;
    for (y = 0; y < vis->height; ++y) {
        assert(linelen <= impl->linelen);
        memcpy(dst, src, linelen);
        src += linelen;
        dst += impl->linelen;
    }
}

void fb_free_visual(struct fb_visual* vis) {
    fbp_visual_unref(vis->_private.impl);
    vis->_private.impl = NULL;
}

void fb_abs2rel(struct fb_visual* vis,
                unsigned* xrel, unsigned* yrel,
                unsigned xabs, unsigned yabs) {
    *xrel = (xabs > vis->x_origin) ? (xabs - vis->x_origin) : 0;
    *yrel = (yabs > vis->y_origin) ? (yabs - vis->y_origin) : 0;
}

void fb_get_fs_visual(struct fb_visual* vis, const struct fbdev* dev) {
    vis->_private.impl = NULL;
    vis->data = dev->fb;
    vis->format = dev->format;
    vis->x_origin = 0;
    vis->y_origin = 0;
    vis->width = dev->var.xres;
    vis->height = dev->var.yres;
    vis->stride = dev->var.xres_virtual;
}

void fb_get_shadow_visual(struct fb_visual* vis, const struct fb_visual* src) {
    vis->_private.impl = fbp_visual_new(src);
    vis->data = vis->_private.impl->buf;
    vis->format = src->format;
    vis->x_origin = src->x_origin;
    vis->y_origin = src->y_origin;
    vis->width = src->width;
    vis->height = src->height;
    vis->stride = src->width;
}

void fb_get_win_visual(struct fb_visual* vis, const struct fb_visual* src,
                       unsigned x, unsigned y, unsigned w, unsigned h) {
    vis->_private.impl = fbp_visual_ref(src->_private.impl);
    if (x > src->width) x = src->width;
    if (y > src->height) y = src->height;
    if (w == 0 || w > src->width - x) w = src->width - x;
    if (h == 0 || h > src->height - y) h = src->height - y;
    vis->data = (uint8_t*)src->data + (x + y * src->stride) * fb_get_pixel_size(src->format);
    vis->format = src->format;
    vis->x_origin = src->x_origin + x;
    vis->y_origin = src->y_origin + y;
    vis->width = w;
    vis->height = h;
    vis->stride = src->stride;
}
