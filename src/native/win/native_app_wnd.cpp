//
// native_app_wnd.cpp
// 
// Native application window implementation for MS Windows.
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 09.05.2021   tstih
// 
#include "native_app_wnd.hpp"

namespace nice {

//{{BEGIN.DEF}}
    native_app_wnd::native_app_wnd(app_wnd *window) : 
        native_wnd(window) {
    }

    native_app_wnd::~native_app_wnd() {}

    void native_app_wnd::show() const { 
        ::ShowWindow(hwnd_, SW_SHOWNORMAL); 
    }
//{{END.DEF}}

} // namespace nice
