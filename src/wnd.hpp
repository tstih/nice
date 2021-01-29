//
// wnd.hpp
// 
// Base window class. 
// 
// (c) 2020 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 17.01.2020   tstih
// 
#ifndef _WND_HPP
#define _WND_HPP

#include "includes.hpp"
#include "resource.hpp"
#include "property.hpp"
#include "signal.h"
#include "geometry.hpp"

namespace nice {

    class wnd : public resource<wnd_instance, 0> {
    public:

        // Ctor(s) and dtor.
        wnd() {}
        virtual ~wnd() { destroy(); }

        // resource implementation.
        virtual wnd_instance create() = 0;        
        void destroy() noexcept override;

        // Methods.
        void repaint(void);

        // Properties.
        property<std::string> title {
            [this](std::string s) { this->set_title(s); },
            [this]() -> std::string {  return this->get_title(); }
        };

        property<size> wsize {
            [this](size sz) { this->set_wsize(sz); },
            [this]() -> size {  return this->get_wsize(); }
        };

        property<pt> location {
            [this](pt p) { this->set_location(p); },
            [this]() -> pt {  return this->get_location(); }
        };

        // Signals.
        signal<> created;
        signal<> destroyed;
        signal<const artist&> paint;
        signal<const resized_info&> resized;
        signal<const mouse_info&> mouse_move;
        signal<const mouse_info&> mouse_down;
        signal<const mouse_info&> mouse_up;

    protected:

        // Setters and getters.
        virtual std::string get_title();
        virtual void set_title(std::string s);
        virtual size get_wsize();
        virtual void set_wsize(size sz);
        virtual pt get_location();
        virtual void set_location(pt location);

    private:

        std::unique_ptr<native_wnd> native_;

    };

}

#endif // _WND_HPP