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

#ifndef _FBLIB_H_
#define _FBLIB_H_

#define FBLIB_WITH_CAIRO
#define FBLIB_WITH_GL
#define FBLIB_WITH_SIM

#ifdef __cplusplus
extern "C" {
#endif

/* Standard API */

struct fbdev;

enum fb_format {
    FB_INVALID = -1,
    FB_RGB565 = 1,
    FB_RGB888 = 2,
};

struct fb_visual_impl;

struct fb_visual {
    void*                   data;
    enum fb_format          format;
    unsigned                x_origin;
    unsigned                y_origin;
    unsigned                width;
    unsigned                height;
    unsigned                stride;
    struct {
        struct fb_visual_impl* impl;
    } _private;
};

struct fbdev* fb_open(const char* devname);
void fb_close(struct fbdev* dev);

unsigned fb_get_pixel_size(enum fb_format fmt);

void fb_get_fs_visual(struct fb_visual* vis, const struct fbdev* dev);
void fb_get_shadow_visual(struct fb_visual* vis, const struct fb_visual* src);
void fb_get_win_visual(struct fb_visual* vis, const struct fb_visual* src,
                       unsigned x, unsigned y, unsigned w, unsigned h);
void fb_commit_visual(struct fb_visual* vis);
void fb_free_visual(struct fb_visual* vis);

void fb_abs2rel(struct fb_visual* vis,
                unsigned* xrel, unsigned* yrel,
                unsigned xabs, unsigned yabs);

/* Cairo API */

#ifdef FBLIB_WITH_CAIRO

typedef struct _cairo_surface cairo_surface_t;

cairo_surface_t* fb_cairo_surface_create(const struct fb_visual* vis);

#endif /* FBLIB_WITH_CAIRO */

/* OpenGL API */

#ifdef FBLIB_WITH_GL

struct fbdev_gl;

struct fbdev_gl* fb_gl_create_context(struct fbdev* dev);
struct fbdev_gl* fb_gl_share_context(struct fbdev_gl* glctx);
void fb_gl_destroy_context(struct fbdev_gl* glctx);
void fb_gl_make_current(struct fbdev_gl* glctx, const struct fb_visual* vis);

#endif /* FBLIB_WITH_GL */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _FBLIB_H_ */
