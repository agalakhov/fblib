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

#include "shmem.h"

#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

static char shmname[1024] = { 0 };
static void* shmem = NULL;
static size_t shmsize = 0;

static void shm_atexit(void) {
    if (shmem) munmap(shmem, shmsize);
    shmem = NULL;
    if (*shmname) shm_unlink(shmname);
    *shmname = 0;
}

static int init_shm(int fd, size_t size) {
    unsigned i;
    shmsize = size;
    if (ftruncate(fd, shmsize) < 0)
        return errno;
    shmem = mmap(NULL, shmsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmem == MAP_FAILED) {
        shmem = NULL;
        return errno;
    }
    /* simulate uninitialized memory */
    for (i = 0; i < shmsize; ++i)
        ((uint8_t*)shmem)[i] = rand() % 255;
    return 0;
}

void* open_shm(const char* prefix, const char* simname, size_t size) {
    int fd, ret;
    snprintf(shmname, sizeof(shmname), "%s%s", prefix, simname);
    fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        fprintf(stderr, "Unable to open shmem \"%s\": %s\n", shmname, strerror(errno));
        fprintf(stderr, "(is a simulator already running on %s?)\n", simname);
        return NULL;
    }
    atexit(shm_atexit);
    ret = init_shm(fd, size);
    close(fd);
    if (ret) {
        fprintf(stderr, "Unable to map shmem: %s\n", strerror(ret));
        return NULL;
    }
    return shmem;
}
