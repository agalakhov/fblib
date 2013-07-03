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

#include "ts.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


struct ts_event { /* As used in the Sharp Zaurus SL-5000d and SL-5500 */
    long y;
    long x;
    long pressure;
    long long millisecs;
};

static const unsigned max_event = 4096;

static char tsname[1024] = { 0 };

static int fd = -1;
static struct ts_event* evbuf = NULL;
static unsigned rp = 0;
static unsigned blen = 0;

static void ts_atexit(void) {
    if (fd >= 0) close(fd);
    fd = -1;
    if (evbuf) free(evbuf);
    evbuf = NULL;
    rp = 0;
    blen = 0;
    if (*tsname) unlink(tsname);
}

int ts_init(const char* simname) {
    evbuf = (struct ts_event*)calloc(sizeof(struct ts_event), max_event);
    if (! evbuf) {
        fprintf(stderr, "Error allocating circular buffer: %s\n", strerror(errno));
        return -1;
    }
    atexit(ts_atexit);
    fd = -1;
    rp = 0;
    blen = 0;
    snprintf(tsname, sizeof(tsname), "%s", simname);
    if (! *tsname) return 0;
    if (mkfifo(tsname, 0600) < 0) {
        fprintf(stderr, "Error creatinig fifo \"%s\": %s\n", tsname, strerror(errno));
        *tsname = '\0';
        return -1;
    }
    return 0;
}

static inline void try_open(void) {
    if (! evbuf) return;
    if (! *tsname) return;
    fd = open(tsname, O_WRONLY | O_NONBLOCK);
    if (fd < 0) return;
}

static inline void try_flush(void) {
    if (fd < 0) return;
    while (blen > 0) {
        ssize_t s = write(fd, evbuf + rp, sizeof(struct ts_event));
        if (s < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            close(fd);
            fd = -1;
            rp = 0;
            blen = 0;
            return;
        }
        rp = (rp + 1) % max_event;
        --blen;
    }
}

void ts_event(unsigned x, unsigned y, unsigned pressure, uint64_t time) {
    unsigned wp;
    struct ts_event ev = { y, x, pressure, time };
    if (fd < 0) try_open();
    if (fd < 0) return;
    wp = (rp + blen) % max_event;
    evbuf[wp] = ev;
    blen++;
    if (blen > max_event) blen = max_event;
    if (blen > 0) try_flush();
}

void ts_idle(void) {
    if (fd < 0) return;
    if (blen > 0) try_flush();
}
