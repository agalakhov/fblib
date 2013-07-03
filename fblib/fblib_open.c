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

#ifdef FBLIB_WITH_SIM
#include "fblib_simfb.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>

static const char* FBENVVARS[] = {
    "FRAMEBUFFER",
    "TSLIB_FBDEV",
    NULL
};

static const char* CONENVVARS[] = {
    "TSLIB_CONSOLEDEVICE",
    NULL
};

unsigned fb_get_pixel_size(enum fb_format fmt) {
    switch (fmt) {
    case FB_RGB565:
        return 2;
    case FB_RGB888:
        return 3;
    case FB_INVALID:
        break;
    }
    return 1; /* not reached */
}

static const char* fbp_get_pixel_name(enum fb_format fmt) {
    switch(fmt) {
    case FB_RGB565:
        return "16[R5G6B5]";
    case FB_RGB888:
        return "24[R8G8B8]";
    case FB_INVALID:
        break;
    }
    return "8[INVALID]";
}

static int fbp_opentty(struct fbdev* dev) {
    int fd, nr;
    int err, ret;
    const char* devname;
    char vtname[128];
    struct vt_stat vts;
    dev->fd_con = -1;
    dev->last_vt = -1;
    devname = fbp_getenv(CONENVVARS, "/dev/tty", NULL);
    if (!strcmp(devname, "none")) return 0;

    snprintf(vtname, sizeof(vtname), "%s%d", devname, 1);
    fd = open(vtname, O_WRONLY);
    err = errno;
    if (fd < 0) goto err_return;
    ret = ioctl(fd, VT_OPENQRY, &nr);
    err = errno;
    close(fd);
    if (ret < 0) goto err_return;

    snprintf(vtname, sizeof(vtname), "%s%d", devname, nr);
    dev->fd_con = open(vtname, O_RDWR | O_NDELAY);
    err = errno;
    if (dev->fd_con < 0) goto err_return;

    if (ioctl(dev->fd_con, VT_GETSTATE, &vts) == 0)
        dev->last_vt = vts.v_active;

    if (ioctl(dev->fd_con, VT_ACTIVATE, nr) < 0) goto err_close;
    if (ioctl(dev->fd_con, VT_WAITACTIVE, nr) < 0) goto err_close;
    if (ioctl(dev->fd_con, KDSETMODE, KD_GRAPHICS) < 0) goto err_close;;

    errno = 0;
    return 0;

err_close:
    err = errno;
    close(dev->fd_con);
err_return:
    errno = err;
    dev->fd_con = -1;
    return -1;
}

static int fbp_is(const struct fb_bitfield* f, uint32_t off, uint32_t len, int msbr) {
    if (f->offset != off) return 0;
    if (f->length != len) return 0;
    if (msbr < 0) return 1;
    if ((f->msb_right ? 1 : 0) != (msbr ? 1 : 0)) return 0;
    return 1;
}

static enum fb_format fbp_determine(const struct fb_var_screeninfo* var) {
    switch (var->bits_per_pixel) {
    case 16:
        if (fbp_is(&var->red, 11, 5, 0) &&
            fbp_is(&var->green, 5, 6, 0) &&
            fbp_is(&var->blue, 0, 5, 0) &&
            fbp_is(&var->transp, 0, 0, -1)
           ) return FB_RGB565;
        break;
    case 24:
        if (fbp_is(&var->red, 16, 8, 0) &&
            fbp_is(&var->green, 8, 8, 0) &&
            fbp_is(&var->blue, 0, 8, 0) &&
            fbp_is(&var->transp, 0, 0, -1)
           ) return FB_RGB888;
        break;
    default:
        break;
    }
    return FB_INVALID;
}

#ifdef FBLIB_WITH_SIM
static struct fbdev* fbp_opensim(const char* simname) {
    int err, ret;
    char shmname[1024];
    struct fbdev* dev;
    struct fblib_sim_shm* mem;
    int shm;
    struct stat st;
    size_t shmlen;

    dev = calloc(1, sizeof(struct fbdev));
    err = ENOMEM;
    if (! dev) goto err_return;

    dev->fd = -1;
    dev->fd_con = -1;
    dev->last_vt = -1;
    dev->fix.smem_start = sizeof(struct fblib_sim_shm);

    snprintf(shmname, sizeof(shmname), FBLIB_SHM "%s", simname);
    shm = shm_open(shmname, O_RDWR, 0600);
    if (shm < 0) goto err_free;
    ret = fstat(shm, &st);
    if (ret < 0) goto err_close;
    shmlen = st.st_size;
    errno = EILSEQ;
    if (shmlen < sizeof(struct fblib_sim_shm) + 4096) goto err_close;
    dev->fb_base = mmap(NULL, shmlen, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    mem = (struct fblib_sim_shm*)(dev->fb_base);
    if (MAP_FAILED == dev->fb_base) goto err_close;
    close(shm);

    dev->fb  = mem->buf;
    dev->fix = mem->fix;
    dev->var = mem->var;
    err = EILSEQ;
    if (dev->fix.smem_len != shmlen) goto err_munmap;
    dev->format = fbp_determine(&dev->var);
    err = EINVAL;
    if (FB_INVALID == dev->format) goto err_munmap;

    printf("Opened simulated framebuffer %s (%ux%u,%s)\n",
           simname, dev->var.xres, dev->var.yres, fbp_get_pixel_name(dev->format));

    errno = 0;
    return dev;

err_munmap:
    munmap(dev->fb_base, shmlen);
    errno = err;
err_close:
    err = errno;
    close(shm);
    errno = err;
err_free:
    err = errno;
    fprintf(stderr, "Unable to open simulated framebuffer %s: %s\n", simname, strerror(err));
    free(dev);
err_return:
    errno = err;
    return NULL;
}
#endif

static void fbp_closetty(struct fbdev* dev) {
    if (dev->fd_con < 0) return;
    ioctl(dev->fd_con, KDSETMODE, KD_TEXT);
    if (dev->last_vt >= 0)
        ioctl(dev->fd_con, VT_ACTIVATE, dev->last_vt);
    close(dev->fd_con);
}

struct fbdev* fb_open(const char* devname) {
    int err, ret;
    struct fbdev* dev;
    unsigned long off;

#ifdef FBLIB_WITH_SIM
    const char* simname = getenv(FBLIB_SIMENV);
    if (simname && *simname)
        return fbp_opensim(simname);
#endif

    devname = fbp_getenv(FBENVVARS, "/dev/fb0", devname);
    dev = calloc(1, sizeof(struct fbdev));
    err = ENOMEM;
    if (! dev) goto err_return;

    ret = fbp_opentty(dev);
    err = errno;
    if (ret < 0) goto err_return;

    dev->fd = open(devname, O_RDWR);
    if (dev->fd < 0) goto err_free;

    ret = ioctl(dev->fd, FBIOGET_FSCREENINFO, &dev->fix);
    if (ret < 0) goto err_close;

    ret = ioctl(dev->fd, FBIOGET_VSCREENINFO, &dev->var);
    if (ret < 0) goto err_close;

    dev->format = fbp_determine(&dev->var);
    errno = EINVAL;
    if (FB_INVALID == dev->format) goto err_close;

    dev->fb_base = mmap(NULL, dev->fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, 0);
    if (MAP_FAILED == dev->fb_base) goto err_close;

    off = (unsigned long)dev->fix.smem_start % (unsigned long) getpagesize();
    dev->fb = dev->fb_base + off;

    errno = 0;
    return dev;

err_close:
    err = errno;
    close(dev->fd);
    errno = err;
err_free:
    err = errno;
    fbp_closetty(dev);
    free(dev);
err_return:
    errno = err;
    return NULL;
}

void fb_close(struct fbdev* dev) {
    munmap(dev->fb_base, dev->fix.smem_len);
    if (dev->fd >= 0)
        close(dev->fd);
    fbp_closetty(dev);
    free(dev);
}

