
//
// app.hpp
// 
// Class encapsulating application entry point.
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 05.05.2021   tstih
// 
#ifndef _APP_HPP
#define _APP_HPP

namespace nice {

//{{BEGIN.DEC}}
    class app {
    public:
        // Cmd line arguments.
        static std::vector<std::string> args;

        // Return code.
        static int ret_code;

        // Application (process) id.
        static app_id id();

        // Application name. First cmd line arg without extension.
        static std::string name();

        // Application instance.
        static app_instance instance();

        // Is another instance already running?
        static bool is_primary_instance();

        // Main desktop application loop.
        static void run(const app_wnd& w);

    private:
        static bool primary_;
        static app_instance instance_;     
    };
//{{END.DEC}}

//{{BEGIN.DEF}}

    int app::ret_code = 0;
    std::vector<std::string> app::args;
    bool app::primary_ = false;
    app_instance app::instance_;

//{{END.DEF}}

}

#endif // _APP_HPP