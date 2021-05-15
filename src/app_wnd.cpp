//
// app_wnd.cpp
// 
// Application window class implementation 
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 09.05.2021   tstih
// 
#include "app_wnd.hpp"

namespace nice {
//{{BEGIN.DEF}}
    bool app_wnd::on_destroy() {
        return true;
    }   

    native_app_wnd* app_wnd::native() {
        return static_cast<native_app_wnd*>(wnd::native());
    }

    void app_wnd::show() {
        // TODO: native()->show();
    }
//{{END.DEF}}
}