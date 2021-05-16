//
// native_artist.cpp
// 
// Encapsulate Windows GDI.
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 05.05.2021   tstih
// 
#include "nice.hpp"

namespace nice {

//{{BEGIN.DEF}}
    void artist::draw_line(color c, pt p1, pt p2) const {
        HPEN pen = ::CreatePen(PS_SOLID, 1, RGB(c.r, c.g, c.b));
        ::SelectObject(canvas_, pen);
        POINT pt;
        ::MoveToEx(canvas_, p1.x, p1.y, &pt);
        ::LineTo(canvas_, p2.x, p2.y);
        ::DeleteObject(pen);
    }

    void artist::draw_rect(color c, rct r) const {
        RECT rect{ r.left, r.top, r.x2(), r.y2() };
        HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
        ::FrameRect(canvas_, &rect, brush);
        ::DeleteObject(brush);
    }

    void artist::fill_rect(color c, rct r) const {   
    }
//{{END.DEF}}
}