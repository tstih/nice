//
// types.hpp
// 
// Mapping standard nice types to MS Windows types.
// 
// (c) 2020 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 16.01.2020   tstih
// 
#ifndef _TYPES_HPP
#define _TYPES_HPP

#include "includes.hpp"

//{{BEGIN.DECL}}
namespace nice {

    // Mapped to Win32 process id.
    typedef DWORD  app_id;

    // Mapped to Win32 application instace (passed to WinMain)
    typedef HINSTANCE app_instance;

    // Mapped to window handle.
    typedef HWND wnd_instance;

    // Screen coordinate for all geometry functions.
    typedef LONG coord;

    // 8 bit integer.
    typedef BYTE byte;

    // Mapped to device context.
    typedef HDC canvas;

}
//{{END.DECL}}

#endif // _TYPES_HPP