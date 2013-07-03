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

unsigned clip(int x, unsigned a) {
    if (x < 0) return 0;
    if ((unsigned)x > a) return a;
    return x;
}


unsigned mode2size(enum m_format mode) {
    switch (mode) {
    case M_RGB888:
        return 3;
    case M_RGB565:
        return 2;
    default:
        return 4;
    }
}
