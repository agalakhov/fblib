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

#include <errno.h>
#include <stdlib.h>

#include <GL/osmesa.h>

struct fbdev_gl {
    struct fbdev* dev;
    OSMesaContext ctx;
};


struct fbdev_gl* fbp_glnew2(struct fbdev* dev, struct fbdev_gl* shared) {
    int err;
    struct fbdev_gl* glctx;
    GLenum fmt = OSMESA_RGB;
    switch (dev->format) {
    case FB_RGB565:
        fmt = OSMESA_RGB_565;
        break;
    case FB_RGB888:
        fmt = OSMESA_RGB;
        break;
    case FB_INVALID:
    default:
        errno = EINVAL;
        return NULL;
    }

    glctx = calloc(1, sizeof(struct fbdev_gl));
    err = ENOMEM;
    if (! glctx) goto err_return;
    glctx->dev = dev;

    glctx->ctx = OSMesaCreateContext(fmt, shared ? shared->ctx : NULL);
    errno = EINVAL;
    if (glctx->ctx == NULL) goto err_free;

    errno = 0;
    return glctx;

err_free:
    err = errno;
    free(glctx);
err_return:
    errno = err;
    return NULL;
}

struct fbdev_gl* fb_gl_create_context(struct fbdev* dev) {
    return fbp_glnew2(dev, NULL);
}

struct fbdev_gl* fb_gl_share_context(struct fbdev_gl* glctx) {
    return fbp_glnew2(glctx->dev, glctx);
}

void fb_gl_make_current(struct fbdev_gl* glctx, const struct fb_visual* vis) {
    GLenum fmt = GL_UNSIGNED_BYTE;
    switch (glctx->dev->format) {
    case FB_RGB565:
        fmt = GL_UNSIGNED_SHORT_5_6_5;
        break;
    case FB_RGB888:
        fmt = GL_UNSIGNED_BYTE;
        break;
    case FB_INVALID:
        return;
    }
    OSMesaMakeCurrent(glctx->ctx, vis->data, fmt, vis->width, vis->height);
    OSMesaPixelStore(OSMESA_ROW_LENGTH, vis->stride);
    OSMesaPixelStore(OSMESA_Y_UP, 0);
}

void fb_gl_destroy_context(struct fbdev_gl* glctx) {
    OSMesaDestroyContext(glctx->ctx);
    free(glctx);
}

