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

#include "x11.h"
#include "common.h"
#include "blit.h"
#include "ts.h"

#include "fblib_simfb.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

static uint32_t* imbuf = NULL;

static Atom wm_protocols, wm_delete_window;

static int is_exit(const XClientMessageEvent* ev) {
    if (ev->format != 32) return 0;
    if (ev->message_type != wm_protocols) return 0;
    if ((Atom)ev->data.l[0] != wm_delete_window) return 0;
    return 1;
}

static inline uint64_t getms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void x_atexit(void) {
    if (imbuf) free(imbuf);
    imbuf = NULL;
}

int x_main(const struct fblib_sim_shm* shm, int argc, char** argv) {
    const unsigned width  = shm->var.xres;
    const unsigned height = shm->var.yres;
    const float diagonal  = (float)M_DIAG(shm->var.reserved) / 100;
    char name[1024];
    int done;
    Window win;
    XEvent ev;
    Display* dpy;
    XImage* image;
    GC gc;
    XSizeHints size;
    uint64_t ms, ms0;
    unsigned pressure = 0;

    imbuf = calloc(1, sizeof(uint32_t) * width * height);
    if (! imbuf) return 1;
    atexit(x_atexit);

    dpy = XOpenDisplay(NULL);
    if (! dpy) {
        fprintf(stderr, "unable to connect to display\n");
        return 7;
    }
    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
                              0, 0, width, height,
                              0, 0, 0);
    snprintf(name, sizeof(name), "Framebuffer: %ux%u %1.1f''", width, height, diagonal);

    size.flags = PMinSize | PMaxSize;
    size.min_width = size.max_width = width;
    size.min_height = size.max_height = height;
    XSetStandardProperties(dpy, win, name, NULL, None, argv, argc, &size);

    XSelectInput(dpy, win, ExposureMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);
    XSetWMProtocols(dpy, win, &wm_delete_window, 1);

    gc = DefaultGC(dpy, DefaultScreen(dpy));

    image = XCreateImage(dpy, CopyFromParent, 24, ZPixmap, 0, (char*)imbuf, width, height, 32, 0);

    XMapWindow(dpy, win);

    ms0 = 0;
    for (done = 0; !done; ) {
        while (XPending(dpy) > 0) {
            XNextEvent(dpy, &ev);
            switch(ev.type) {
            case Expose:
                fb2buf(imbuf, shm);
                XPutImage(dpy, win, gc, image, 0, 0, 0, 0, width, height);
                break;
            case ClientMessage:
                done = is_exit(&ev.xclient);
                break;
            case ButtonPress:
                if (ev.xbutton.button == 1) pressure = 16;
                ts_event(clip(ev.xbutton.x, width), clip(ev.xbutton.y, height), pressure, getms());
                break;
            case ButtonRelease:
                if (ev.xbutton.button == 1) pressure = 0;
                ts_event(clip(ev.xbutton.x, width), clip(ev.xbutton.y, height), pressure, getms());
                break;
            case MotionNotify:
                ts_event(clip(ev.xmotion.x, width), clip(ev.xmotion.y, height), pressure, getms());
                break;
            default:
                break;
            }
        }
        ms = getms();
        if (ms - ms0 > 20) {
            if (fb2buf(imbuf, shm)) {
                XPutImage(dpy, win, gc, image, 0, 0, 0, 0, width, height);
            }
            ms0 = ms;
        }
        ts_idle();
        usleep(1000);
    }

    XDestroyImage(image);
    imbuf = NULL; /* deleted by XDestroyImage */
    XCloseDisplay(dpy);
    return 0;
}
