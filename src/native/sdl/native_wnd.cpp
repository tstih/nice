//
// native_wnd.hpp
// 
// Native window for SDL. 
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 02.06.2021   tstih
// 
#include "nice.hpp"

namespace nice {

//{{BEGIN.DEF}}
    // Static variable.
    std::map<SDL_Window*,native_wnd*> native_wnd::wmap_;

    native_wnd::native_wnd(wnd *window) {
        window_=window;
    }

    native_wnd::~native_wnd() {
        // And lazy destroy. 
        ::SDL_DestroyWindow(winst_);
    }

    void native_wnd::repaint() {
        // TODO: Whatever.
    }

    void native_wnd::set_title(std::string s) {
        ::SDL_SetWindowTitle(winst_, s.c_str());
    };
    
    std::string native_wnd::get_title() {
        return SDL_GetWindowTitle(winst_);
    }

    size native_wnd::get_wsize() {
        int w,h;
        ::SDL_GetWindowSize(winst_, &w, &h);
        return { w, h };
    }

    void native_wnd::set_wsize(size sz) {
        // TODO: Check SDL_GetRendererOutputSize
        ::SDL_SetWindowSize(winst_, sz.w, sz.h);
    }

    pt native_wnd::get_location() {
        int x,y;
        SDL_GetWindowPosition(winst_, &x, &y);
        return { x, y };
    }

    void native_wnd::set_location(pt location) {
        SDL_SetWindowPosition(winst_,location.x, location.y);
    }

    void native_wnd::destroy() {
        // Remove me from windows map.
        wmap_.erase (winst_); 
    }

    // TODO:for now SDL only has one window so we're
    // assuming the first entry in the map, but we're
    // ready for more!
    bool native_wnd::global_wnd_proc(const SDL_Event& e) {
        native_wnd* nw = wmap_.begin()->second;
        return nw->local_wnd_proc(e);
    }

    // Local (per window) window proc.
    bool native_wnd::local_wnd_proc(const SDL_Event& e) {
        bool quit=false;
        // TODO: process events and delegate to signals.
        return quit;
    }
//{{END.DEF}}

} // namespace nice