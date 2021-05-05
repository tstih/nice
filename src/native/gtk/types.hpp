//
// types.hpp
// 
// Mapping standard nice types to MS Windows types.
// TODO: Use Cairo for painting.
// 
// (c) 2020 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 16.01.2020   tstih
// 
#ifndef _TYPES_HPP
#define _TYPES_HPP

#include "includes.hpp"

//{{BEGIN.DEC}}
namespace nice {

    // Unix process id.
    typedef pid_t app_id;

    // Basic GTK stuff.
    typedef GtkApplication* app_instance; // Not used.

    // Window instance.
    typedef GtkWidget*  wnd_instance;
    #define WND_NULL nullptr

    // Coordinate.
    typedef int coord;

    // 8 bit integer.
    typedef uint8_t byte;

    // lib cairo and required stuff.
    typedef cairo_t* canvas;

}
//{{END.DEC}}

#endif // _TYPES_HPP