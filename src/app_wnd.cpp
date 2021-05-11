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
        if (native_==nullptr)
            native_=std::make_unique<native_app_wnd>(this);
        return native_.get();
    }

    void app_wnd::show() {
        native()->show();
    }
//{{END.DEF}}
}