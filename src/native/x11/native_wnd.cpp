//
// native_wnd.cpp
// 
// Native window implementation for X11.
// 
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 17.05.2021   tstih
// 
#include "nice.hpp"

namespace nice {

//{{BEGIN.DEF}}
    // Static variable.
    std::map<Window,native_wnd*> wmap_;

    native_wnd::native_wnd(wnd *window) {
        window_=window;
        display_=app::instance().display;
    }

    native_wnd::~native_wnd() {
        // And lazy destroy. We could do this in the destroy() function.
        XDestroyWindow(display_, winst_); winst_=0;
    }

    void native_wnd::repaint() {
        XClearArea(display_, winst_, 0, 0, 1, 1, true);
    }

    void native_wnd::set_title(std::string s) {
        ::XStoreName(display_, winst_, s.c_str());
    };
    
    std::string native_wnd::get_title() {
        // XFetchName and XFree...
    }

    size native_wnd::get_wsize() {
        // XGetWindowAttributes
    }

    void native_wnd::set_wsize(size sz) {
        XResizeWindow(display_,winst_, sz.w, sz.h);
    }

    pt native_wnd::get_location() {
    }

    void native_wnd::set_location(pt location) {
        XMoveWindow(display_,winst_, location.left, location.top);
    }

    void native_wnd::destroy() {
        // Remove me from windows map.
        wmap_.erase (winst_); 
    }

    // Static (global) window proc. For all classes -
    // Remaps the call to local window proc.
    bool native_wnd::global_wnd_proc(const XEvent& e) {
        Window xw = e.xany.window;
        native_wnd* nw = wmap_[xw];
        return nw->local_wnd_proc(e);
    }

    // Local (per window) window proc.
    bool native_wnd::local_wnd_proc(const XEvent& e) {
        bool quit=false;
        switch ( e.type )
        {
        case CreateNotify:
            window_->created.emit();
            break;
        case ClientMessage:
            {
                Atom atom = XInternAtom ( display_,
                            "WM_DELETE_WINDOW",
                            false );
                if ( atom == e.xclient.data.l[0] )
                    window_->destroyed.emit();
                    quit=true;
            }
            break;
        case Expose:
            {
                canvas c { 
                    display_, 
                    winst_, 
                    XCreateGC(display_, winst_, 0, NULL) }; 
                artist a(c);
                window_->paint.emit(a);
                XFreeGC(display_,c.gc);
            }
		    break;
        case ButtonPress: // https://tronche.com/gui/x/xlib/events/keyboard-pointer/keyboard-pointer.html
        case ButtonRelease:
            {
            mouse_info mi = {
                { e.xmotion.x, e.xmotion.y },
                e.xbutton.button&Button1, // lmrcsa
                e.xbutton.button&Button2,
                e.xbutton.button&Button3,
                e.xbutton.state&ControlMask,
                e.xbutton.state&ShiftMask
            };
            if (e.type==ButtonPress)
                window_->mouse_down.emit(mi);
            else
                window_->mouse_up.emit(mi);
            }
            break;
        case MotionNotify:
            {
            mouse_info mi = {
                { e.xmotion.x, e.xmotion.y },
                e.xmotion.state&Button1Mask, // lmrcsa
                e.xmotion.state&Button2Mask,
                e.xmotion.state&Button3Mask,
                e.xmotion.state&ControlMask,
                e.xmotion.state&ShiftMask
            };
            window_->mouse_move.emit(mi);
            }
            break;
        case KeyPress:
            break;
        case KeyRelease:
            break;
        } // switch
        return quit;
    }
//{{END.DEF}}

} // namespace nice