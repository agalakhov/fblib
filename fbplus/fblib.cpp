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

#include "fblib.hpp"
#include "fblib.h"

#include <cerrno>
#include <cstring>

namespace Fb {

class Visual::Shadow : public Visual { };

const char* Exception::what() const throw() {
    return strerror(m_err);
}

Device::Device(const char* devname) throw(Exception)
    : m_device(fb_open(devname))
{
    if (! m_device)
        throw Exception(errno);
}

Device::~Device() throw() {
    fb_close(m_device);
}

Visual::Visual(const Device& dev) throw() {
    fb_get_fs_visual(&m_visual, dev.m_device);
}

Visual::Visual(const Visual::Shadow& src) throw() {
    fb_get_shadow_visual(&m_visual, &static_cast<const Visual&>(src).m_visual);
}

Visual::Visual(const Visual& src) throw() {
    fb_get_win_visual(&m_visual, &src.m_visual, 0, 0, 0, 0);
}

Visual::Visual(const Visual& src, unsigned x, unsigned y, unsigned width, unsigned height) throw() {
    fb_get_win_visual(&m_visual, &src.m_visual, x, y, width, height);
}

Visual::~Visual() throw() {
    fb_free_visual(&m_visual);
}

const Visual& Visual::operator= (const Visual& src) throw() {
    fb_free_visual(&m_visual);
    fb_get_win_visual(&m_visual, &src.m_visual, 0, 0, 0, 0);
    return *this;
}

const Visual::Shadow& Visual::shadow() const throw() {
    return static_cast<const Shadow&>(*this);
}

void Visual::commit() throw() {
    fb_commit_visual(&m_visual);
}

#ifdef FBLIB_WITH_CAIRO
Cairo::RefPtr<Cairo::Surface> Visual::Cairo() const {
    return Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(fb_cairo_surface_create(&m_visual)));
}
#endif

} // namespace Fb
