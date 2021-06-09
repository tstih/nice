//
// native_artist.cpp
// 
// Encapsulate X11 drawing.
// TODO:
//  Use Cairo.
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 17.05.2021   tstih
// 
#include "nice.hpp"

namespace nice {

//{{BEGIN.DEF}}
    void artist::draw_line(color c, pt p1, pt p2) const {
    }

    void artist::draw_rect(color c, rct r) const {   
    }

    void artist::fill_rect(color c, rct r) const {   

        auto screen=DefaultScreen(canvas_.d);

        Colormap cmap=DefaultColormap(canvas_.d,screen);    
        XColor xcolour;

        // I guess XParseColor will work here
        xcolour.red = 32000; xcolour.green = 65000; xcolour.blue = 32000;
        xcolour.flags = DoRed | DoGreen | DoBlue;
        XAllocColor(canvas_.d, cmap, &xcolour);

        XSetForeground(canvas_.d, canvas_.gc, xcolour.pixel);

        // TODO:
        XFillRectangle( canvas_.d, canvas_.w, canvas_.gc, r.x, r.y, 200, 200 );
    }

    void artist::draw_raster(const raster& rst, pt p) const {
    }
//{{END.DEF}}
}