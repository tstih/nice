//
// native_includes.hpp
// 
// Platform dependant includes.
// 
// (c) 2020 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 16.01.2020   tstih
// 

//{{BEGIN.INC}}
extern "C" {
#define nice unix_nice
#include <unistd.h>
#undef nice
#include <stdint.h>
#include <fcntl.h>
#include <sys/file.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
}
//{{END.INC}}