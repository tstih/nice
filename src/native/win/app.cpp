
//
// app.cpp
// 
// Application entry point & logic code.
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 05.05.2021   tstih
// 
#include "app.h"

namespace nice {

//{{BEGIN.DEF}}
    app_id app::id() {
        return ::GetCurrentProcessId();
    }

    std::string app::name() {
        return std::filesystem::path(args[0]).stem().string();
    }

    app_instance app::instance() {
        return instance_;
    }

    bool app::is_primary_instance() {
        // Are we already primary instance? If not, try to become one.
        if (!primary_) {
            std::string aname = app::name();
            // Create local mutex.
            std::ostringstream name;
            name << "Local\\" << aname;
            ::CreateMutex(0, FALSE, name().str().c_str());
            // We are primary instance.
            primary_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
        }
        return primary_;
    }

    void app::run(const app_wnd& w) {
        // Show window.
        ::ShowWindow(w.instance(), SW_SHOWNORMAL);

        // Message loop.
        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        // Finally, set the return code.
        ret_code = (int)msg.wParam;
    }
//{{END.DEF}}

}