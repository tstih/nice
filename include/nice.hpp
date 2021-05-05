/*
 * nice.hpp
 * 
 * (Single) header file for the nice GUI library.
 * 
 * (c) 2020 Tomaz Stih
 * This code is licensed under MIT license (see LICENSE.txt for details).
 * 
 * 18.12.2020   tstih
 * 
 */
#ifndef _NICE_HPP
#define _NICE_HPP

#ifdef __GTK__
extern "C" {
#include <sys/file.h>
#include <unistd.h>
#include <gtk/gtk.h>
}

namespace nice {

    // Unix process id.
    typedef pid_t app_id;

    // Basic GTK stuff.
    typedef GtkApplication* app_instance; // Not used.

    // Window instance.
    typedef GtkWidget*  wnd_instance;

    // Coordinate.
    typedef int coord;

    // 8 bit integer.
    typedef uint8_t byte;

    // lib cairo and required stuff.
    typedef cairo_t* canvas;

}

#elif __SDL__




#elif __WIN__
extern "C" {
#include <windows.h>
}

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

#elif __X11__
extern "C" {
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

namespace nice {

    // Unix process id.
    typedef pid_t app_id;

    // Basic X11 stuff.
    typedef struct x11_app_instance {
        Display* d;
    } app_instance;

    // X11 window instance.
    typedef Window wnd_instance;

    // X11 coordinate.
    typedef int coord;

    // 8 bit integer.
    typedef uint8_t byte;

    // X11 GC and required stuff.
    typedef struct x11_canvas {
        Display* d;
        Window w;
        GC gc;
    } canvas;

}

#endif

#include <exception>
#include <string>
#include <functional>
#include <map>


namespace nice {

#define throw_ex(ex, what) \
        throw ex(what, __FILE__,__FUNCTION__,__LINE__);

    class nice_exception : public std::exception {
    public:
        nice_exception(
            std::string what,
            std::string file = nullptr,
            std::string func = nullptr,
            int line = 0) : what_(what), file_(file), func_(func), line_(line) {};
        std::string what() { return what_; }
    protected:
        std::string what_;
        std::string file_; // __FILE__
        std::string func_; // __FUNCTION__
        int line_; // __LINE__
    };

    template <typename... Args>
    class signal {
    public:
        signal() : current_id_(0) {}
        signal(std::function<void()> init) : signal() { init_ = init; }
        template <typename T> int connect(T* inst, bool (T::* func)(Args...)) {
            return connect([=](Args... args) {
                return (inst->*func)(args...);
                });
        }

        template <typename T> int connect(T* inst, bool (T::* func)(Args...) const) {
            return connect([=](Args... args) {
                return (inst->*func)(args...);
                });
        }

        int connect(std::function<bool(Args...)> const& slot) const {
            if (!initialized_ && init_ != nullptr) { init_(); initialized_ = true; }
            slots_.insert(std::make_pair(++current_id_, slot));
            return current_id_;
        }

        void disconnect(int id) const {
            slots_.erase(id);
        }

        void disconnect_all() const {
            slots_.clear();
        }

        void emit(Args... p) {
            // Iterate in reverse order to first emit to last connections.
            for (auto it = slots_.rbegin(); it != slots_.rend(); ++it) {
                if (it->second(std::forward<Args>(p)...)) break;
            }
        }
    private:
        mutable std::map<int, std::function<bool(Args...)>> slots_;
        mutable int current_id_;
        mutable bool initialized_{ false };
        std::function<void()> init_{ nullptr };
    };

   class percent
    {
        double percent_;
    public:
        class pc {};
        explicit constexpr percent(pc, double dpc) : percent_{ dpc } {}
    };

    class pixel
    {
        int pixel_;
    public:
        class px {};
        explicit constexpr pixel(px, int ipx) : pixel_{ ipx } {}
        int value() { return pixel_; }
    };

    template<typename T, T N = nullptr>
    class resource {
    public:
        // Create and destroy pattern.
        virtual T create() = 0;
        virtual void destroy() noexcept = 0;

        // Id setter.
        virtual void instance(T instance) const { instance_ = instance; }

        // Id getter with lazy eval.
        virtual T instance() const {
            // Lazy evaluate by callign create.
            if (instance_ == N)
                instance_ = const_cast<resource<T, N>*>(this)->create();
            // Return.
            return instance_;
        }

        bool initialized() {
            return !(instance_ == N);
        }

    private:
        // Store resource value here.
        mutable T instance_{ N };
    };

    template<typename T>
    class property {
    public:
        property(
            std::function<void(T)> setter,
            std::function<T()> getter) :
            setter_(setter), getter_(getter) { }
        operator T() const { return getter_(); }
        property<T>& operator= (const T& value) { setter_(value); return *this; }
    private:
        std::function<void(T)> setter_;
        std::function<T()> getter_;
    };

    typedef struct size_s {
        union { coord width; coord w; };
        union { coord height; coord h; };
    } size;

    typedef struct color_s {
        byte r;
        byte g;
        byte b;
        byte a;
    } color;

    typedef struct pt_s {
        union { coord left; coord x; };
        union { coord top; coord y; };
    } pt;

    typedef struct rct_s {
        union { coord left; coord x; coord x1; };
        union { coord top; coord y;  coord y1; };
        union { coord width; coord w; };
        union { coord height; coord h; };
        coord x2() { return left + width; }
        coord y2() { return top + height; }
    } rct;

  
    // Buton status: true=down, false=up.
    struct mouse_info {
        pt location;
        bool left_button;       
        bool middle_button;
        bool right_button;
        bool ctrl;
        bool shift;
    };




    constexpr percent operator "" _pc(long double dpc)
    {
        return percent{ percent::pc{}, static_cast<double>(dpc) };
    }

    constexpr pixel operator "" _px(unsigned long long ipx)
    {
        return pixel{ pixel::px{}, static_cast<int>(ipx) };
    }



}

#endif // _NICE_HPP
