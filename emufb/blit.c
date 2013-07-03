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

#include "blit.h"
#include "common.h"

#include "fblib_simfb.h"

#include <stdint.h>

#include <zlib.h>

static inline uint32_t pix565(const void* buf, unsigned i) {
    uint16_t pix = ((const uint16_t*)buf)[i];
    uint32_t r = (0x1F & (pix >> 11)) * 255 / 0x1F;
    uint32_t g = (0x3F & (pix >> 5)) * 255 / 0x3F;
    uint32_t b = (0x1F & pix) * 255 / 0x1F;
    return (r << 16) | (g << 8) | (b);
}

static inline uint32_t pix888(const void* buf, unsigned i) {
    const uint8_t* pix = (const uint8_t*)buf + 3 * i;
    uint32_t r = pix[0];
    uint32_t g = pix[1];
    uint32_t b = pix[2];
    return (r << 16) | (g << 8) | (b);
}

static inline uint32_t pix(const void* buf, unsigned i, enum m_format mode) {
    switch (mode) {
    case M_RGB565:
        return pix565(buf, i);
    case M_RGB888:
        return pix888(buf, i);
    }
    return 0; /* not reached */
}

int fb2buf(uint32_t* image, const struct fblib_sim_shm* shm) {
    const unsigned width   = shm->var.xres;
    const unsigned height  = shm->var.yres;
    const unsigned pixsize = shm->var.bits_per_pixel / 8;
    const enum m_format mode = M_MODE(shm->var.reserved);
    unsigned x, y;
    uint32_t ncrc;
    static uint32_t crc = 0;
    ncrc = crc32(0L, shm->buf, width * height * pixsize);
    if (ncrc == crc) return 0;
    crc = ncrc;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            unsigned i = y * width + x;
            image[i] = pix(shm->buf, i, mode);
        }
    }
    return 1;
}
