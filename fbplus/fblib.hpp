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

#pragma once

#include "fblib.h"

#ifdef FBLIB_WITH_CAIRO
#include <cairomm/cairomm.h>
#endif

#include <exception>

namespace Fb {

class Exception : public std::exception {
public:
    Exception(int err) throw() : m_err(err) { }
    virtual const char* what() const throw();
private:
    int m_err;
};

class Device {
    friend class Visual;
public:
    Device(const char* devname = 0) throw(Exception);
    virtual ~Device() throw();
private:
    Device(const Device&);
    const Device& operator= (const Device&);
private:
    struct fbdev* m_device;
};

class Visual {
public:
    class Shadow;
public:
    Visual(const Device& dev) throw();
    Visual(const Shadow& src) throw();
    Visual(const Visual& src) throw();
    Visual(const Visual& src, unsigned x, unsigned y, unsigned width, unsigned height) throw();
    virtual ~Visual() throw();
    const Visual& operator= (const Visual& src) throw();
    const Shadow& shadow() const throw();

    unsigned x_origin() const throw() { return m_visual.x_origin; }
    unsigned y_origin() const throw() { return m_visual.y_origin; }
    unsigned width()    const throw() { return m_visual.width; }
    unsigned height()   const throw() { return m_visual.height; }

    void commit() throw();

#ifdef FBLIB_WITH_CAIRO
    Cairo::RefPtr<Cairo::Surface> Cairo() const;
#endif

protected:
    const struct fb_visual* cstruct() const throw() { return &m_visual; }
    struct fb_visual* cstruct() throw() { return &m_visual; }

private:
    struct fb_visual m_visual;
};

} // namespace Fb
