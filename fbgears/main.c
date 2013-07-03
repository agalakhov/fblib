/*
 * This code is borrowed from Mesa 3d demos.
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "fblib.h"
#include "gears.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

struct fbdev* dev = NULL;
struct fbdev_gl* gl = NULL;

struct fb_visual fsvis;
struct fb_visual winvis;
struct fb_visual shadow;

static void sig(int sig) {
    if (gl)  fb_gl_destroy_context(gl);
    if (dev) fb_close(dev);
    (void)sig;
    exit(1);
}

static void render(struct fb_visual* vis) {
    fb_gl_make_current(gl, vis);
    init();
    reshape(vis->width, vis->height);
    while(1) {
        double dt = draw();
//        printf("dt=%1.6f  FPS=%3.2f\n", dt, 1.0/dt);
        fb_commit_visual(vis);
    }
}

int main() {
    signal(SIGSEGV, sig);
    signal(SIGINT, sig);
    signal(SIGTERM, sig);
    signal(SIGKILL, sig);

    dev = fb_open(NULL);
    if (! dev) {
        fprintf(stderr, "error opening device: %s\n", strerror(errno));
        return 1;
    }

    gl = fb_gl_create_context(dev);
    if (! gl) {
        fprintf(stderr, "error opening opengl: %s\n", strerror(errno));
        fb_close(dev);
        return 1;
    }

    fb_get_fs_visual(&fsvis, dev);
    fb_get_win_visual(&winvis, &fsvis, 100, 100, 300, 300);
    fb_get_shadow_visual(&shadow, &winvis);

    memset(fsvis.data, 0xFF, fb_get_pixel_size(fsvis.format)*fsvis.width*fsvis.height);

    render(&shadow);

    fb_free_visual(&shadow);
    fb_free_visual(&winvis);
    fb_free_visual(&fsvis);

    fb_gl_destroy_context(gl);
    fb_close(dev);

    return 0;
}
