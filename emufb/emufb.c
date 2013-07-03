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

#include "common.h"
#include "fbinfo.h"
#include "shmem.h"
#include "ts.h"
#include "x11.h"

#include "fblib_simfb.h"

#include <stddef.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

static const char* simname;
static const char* tsname;

static enum m_format mode;
static unsigned width, height;
static float diagonal;

static enum m_format parse_mode(const char* fmt) {
    if (!strcmp(fmt, "RGB565") || !strcmp(fmt, "rgb565"))
        return M_RGB565;
    if (!strcmp(fmt, "RGB888") || !strcmp(fmt, "rgb888"))
        return M_RGB888;
    return 0;
}

static int parse_opts(int argc, char** argv) {
    const char* simvar;

    simname = "fb0";
    simvar = getenv(FBLIB_SIMENV);
    if (simvar && *simvar)
        simname = simvar;

    tsname = "fakets0";

    mode = M_RGB888;
    width = 640;
    height = 480;
    diagonal = 6;

    for (argc -= 1, argv += 1; argc >= 2; argc -= 2, argv += 2) {
        if (!strcmp(argv[0], "-w")) {
            width = clip(atoi(argv[1]), 1000000);
            if (width < 64 || width > 1600) {
                fprintf(stderr, "Invalid width %s\n", argv[1]);
                return 0;
            }
            continue;
        }
        if (!strcmp(argv[0], "-h")) {
            height = clip(atoi(argv[1]), 1000000);
            if (height < 32 || height > 1200) {
                fprintf(stderr, "Invalid height %s\n", argv[1]);
                return 0;
            }
            continue;
        }
        if (!strcmp(argv[0], "-m")) {
            mode = parse_mode(argv[1]);
            if (! mode) {
                fprintf(stderr, "Invalid mode %s\n", argv[1]);
                return 0;
            }
            continue;
        }
        if (!strcmp(argv[0], "-d")) {
            diagonal = atof(argv[1]);
            if (diagonal < 0.5 || diagonal > 100.0) {
                fprintf(stderr, "Invalid diagonal %s\n", argv[1]);
                return 0;
            }
            continue;
        }
        if (!strcmp(argv[0], "-f")) {
            simname = argv[1];
            continue;
        }
        argc = 1;
    }

    if (argc) {
        printf("Usage: emufb [-w width] [-h height] [-m mode] [-f fbname] [-d diagonal]\n");
        printf("Valid modes: RGB565, RGB888\n");
        return 0;
    }

    if (!*simname) {
        fprintf(stderr, "Simulated fb name should not be empty\n");
        return 0;
    }
    if (strspn(simname, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "abcdefghijklmnopqrstuvwxyz"
                        "01234567890_-:+.@#="
              ) != strlen(simname)) {
        fprintf(stderr, "Invalid simulated fb name \"%s\"\n", simname);
        return 0;
    }

    return 1;
}

static void sig(int sig) {
    fprintf(stderr, "caught signal %i\n", sig);
    exit(1);
}

int main(int argc, char** argv) {
    const char* pmode = NULL;
    unsigned pixsize;
    struct fblib_sim_shm* shmem;
    size_t shmsize;

    signal(SIGINT,  sig);
    signal(SIGTERM, sig);
    signal(SIGSEGV, sig);
    signal(SIGKILL, sig);
    signal(SIGPIPE, SIG_IGN);

    if (! parse_opts(argc, argv))
        return 3;

    pixsize = mode2size(mode);
    shmsize = sizeof(struct fblib_sim_shm) + width * height * pixsize;

    shmem = (struct fblib_sim_shm*)open_shm(FBLIB_SHM, simname, shmsize);
    if (! shmem) return 1;

    if (ts_init(tsname)) return 1;

    switch (mode) {
    case M_RGB888:
        pmode = "RGB 24-bit (888)";
        break;
    case M_RGB565:
        pmode = "RGB 16-bit (565)";
        break;
    }
    printf("Started simulated framebuffer %s\n", simname);
    printf("Resolution: %ux%u, %1.1f\", %s\n", width, height, diagonal, pmode);
    printf("export "FBLIB_SIMENV"=%s\n", simname);

    fill_fbinfo(shmem, shmsize, width, height, diagonal, mode);
    return x_main(shmem, argc, argv);
}
