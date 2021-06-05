/*
 * nice.hpp
 * 
 * (Single) header file for the nice GUI library.
 * 
 * (c) 2020 - 2021 Tomaz Stih
 * This code is licensed under MIT license (see LICENSE.txt for details).
 * 
 * 02.06.2021   tstih
 * 
 */
#ifndef _NICE_HPP
#define _NICE_HPP

#ifdef __WIN__
extern "C" {
#include <windows.h>
#include <windowsx.h>
}

#elif __X11__
extern "C" {
#define nice unix_nice
#include <unistd.h>
#undef nice
#include <stdint.h>
#include <fcntl.h>
#include <sys/file.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
}

#elif __SDL__
extern "C" {
#define nice unix_nice
#include <unistd.h>
#undef nice
#include <stdint.h>
#include <fcntl.h>
#include <sys/file.h>

#include <SDL2/SDL.h>
}

#endif

#include <exception>
#include <string>
#include <sstream>
#include <functional>
#include <map>
#include <filesystem>


namespace nice {

#ifdef __WIN__
    // Mapped to Win32 process id.
    typedef DWORD  app_id;

    // Mapped to Win32 application instace (passed to WinMain)
    typedef HINSTANCE app_instance;

    // Screen coordinate for all geometry functions.
    typedef LONG coord;

    // 8 bit integer.
    typedef BYTE byte;

    // Mapped to device context.
    typedef HDC canvas;

#elif __X11__
    // Unix process id.
    typedef pid_t app_id;

    // Basic X11 stuff.
    typedef struct x11_app_instance {
        Display* display;
    } app_instance;

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

#elif __SDL__
    // Unix process id.
    typedef pid_t app_id;

    // Basic X11 stuff.
    typedef int app_instance;

    // X11 coordinate.
    typedef int coord;

    // 8 bit integer.
    typedef uint8_t byte;

    // X11 GC and required stuff.
    typedef SDL_Surface* canvas;

#endif

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

    struct resized_info {
        coord width;
        coord height;
    };

    // Buton status: true=down, false=up.
    struct mouse_info {
        pt location;
        bool left_button;       
        bool middle_button;
        bool right_button;
        bool ctrl;
        bool shift;
    };

    class artist {
    public:
        // Pass canvas instance, don't own it.
        artist(const canvas& canvas) {
            canvas_ = canvas;
        }

        // Methods.
        void draw_line(color c, pt p1, pt p2) const;
        void draw_rect(color c, rct r) const;
        void fill_rect(color c, rct r) const;

    private:
        // Passed canvas.
        canvas canvas_;
    };

    class raster {
    public:
        // Constructs a new raster.        
        raster(int width, int height);
        // Destructs the raster.
        virtual ~raster();
    private:
        int width_, height_, stride_, len_;
        std::unique_ptr<uint8_t> data_;
    };

#ifdef __WIN__
    class wnd; // Forward declaration.
    class native_wnd {
    public:
        // Ctor and dtor.
        native_wnd(wnd *window);
        virtual ~native_wnd();
        // Method(s).
        void destroy(void);
        void repaint(void);
        std::string get_title();
        void set_title(std::string s); 
        size get_wsize();
        void set_wsize(size sz);
        pt get_location();
        void set_location(pt location);
    protected:
        // Window variables.
        HWND hwnd_;
        WNDCLASSEX wcex_;
        std::string class_;
        // Global and local window procedures.
        static LRESULT CALLBACK global_wnd_proc(
            HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        virtual LRESULT local_wnd_proc(
            UINT msg, WPARAM wparam, LPARAM lparam);
        wnd* window_;
    };

    class app_wnd; // Forward declaration.
    class native_app_wnd : public native_wnd {
    public:
        native_app_wnd(
            app_wnd *window,
            std::string title,
            size size
        );
        virtual ~native_app_wnd();
        void show() const;
    };

#elif __X11__
    class wnd; // Forward declaration.
    class native_wnd {
    public:
        // Ctor creates X11 window. 
        native_wnd(wnd *window);
        // Dtor.
        virtual ~native_wnd();
        // Destroy native window.
        void destroy(void);
        // Invalidate native window.
        void repaint(void);
        // Get window title.
        std::string get_title();
        // Set window title.
        void set_title(std::string s);
        // Get window size (not client size). 
        size get_wsize();
        // Set window size.
        void set_wsize(size sz);
        // Get window relative location (to parent)
        // or absolute location if parent is screen. 
        pt get_location();
        // Set window location.
        void set_location(pt location);
        // Global window procedure (static)
        static bool global_wnd_proc(const XEvent& e);
    protected:
        // X11 window structure.
        Window winst_; 
        // X11 display.
        Display* display_;
        // A map from X11 window to native_wnd.
        static std::map<Window,native_wnd*> wmap_;
        // Local window procdure.
        virtual bool local_wnd_proc(const XEvent& e);
        // Pointer to related non-native window struct.
        wnd* window_;
    };

    class app_wnd; // Forward declaration.
    class native_app_wnd : public native_wnd {
    public:
        native_app_wnd(
            app_wnd *window,
            std::string title,
            size size
        );
        virtual ~native_app_wnd();
        void show() const;
    };

#elif __SDL__
    class wnd; // Forward declaration.
    class native_wnd {
    public:
        // Ctor creates X11 window. 
        native_wnd(wnd *window);
        // Dtor.
        virtual ~native_wnd();
        // Destroy native window.
        void destroy(void);
        // Invalidate native window.
        void repaint(void);
        // Get window title.
        std::string get_title();
        // Set window title.
        void set_title(std::string s);
        // Get window size (not client size). 
        size get_wsize();
        // Set window size.
        void set_wsize(size sz);
        // Get window relative location (to parent)
        // or absolute location if parent is screen. 
        pt get_location();
        // Set window location.
        void set_location(pt location);
        // Global window procedure (static)
        static bool global_wnd_proc(const SDL_Event& e);
    protected:
        // Native SDL window structure.
        SDL_Window* winst_; 
        // A map from X11 window to native_wnd.
        static std::map<SDL_Window *,native_wnd*> wmap_;
        // Local window procdure.
        virtual bool local_wnd_proc(const SDL_Event& e);
        // Pointer to related non-native window struct.
        wnd* window_;
    };

    class app_wnd; // Forward declaration.
    class native_app_wnd : public native_wnd {
    public:
        native_app_wnd(
            app_wnd *window,
            std::string title,
            size size
        );
        virtual ~native_app_wnd();
        void show() const;
    };

#endif
    class wnd  {
    public:
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

        // Pimpl. Concrete window must implement this!
        virtual native_wnd* native() = 0;
    };

    class app_wnd : public wnd {
    public:
        app_wnd(std::string title, size size) : native_(nullptr) {
            // Store parameters.
            title_ = title; size_ = size;
            // Subscribe to destroy signal.
            destroyed.connect(this, &app_wnd::on_destroy);
        }
        void show();
        
    protected:
        // Destroyed handler...
        bool on_destroy();         
        // Pimpl implementation.
        virtual native_app_wnd* native() override;
    private:
        std::string title_;
        size size_;
        std::unique_ptr<native_app_wnd> native_;
    };

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

        // Application instance get and set.
        static app_instance instance();
        static void instance(app_instance instance);

        // Is another instance already running?
        static bool is_primary_instance();

        // Main desktop application loop.
        static void run(const app_wnd& w);

    private:
        static bool primary_;
        static app_instance instance_;     
    };


    constexpr percent operator "" _pc(long double dpc)
    {
        return percent{ percent::pc{}, static_cast<double>(dpc) };
    }

    constexpr pixel operator "" _px(unsigned long long ipx)
    {
        return pixel{ pixel::px{}, static_cast<int>(ipx) };
    }


    int app::ret_code = 0;
    std::vector<std::string> app::args;
    bool app::primary_ = false;
    app_instance app::instance_;

    app_instance app::instance() {
        return instance_;
    }

    void app::instance(app_instance instance) {
        instance_ = instance;
    }

    std::string app::name() {
        return std::filesystem::path(args[0]).stem().string();
    }
    void wnd::repaint(void) { native()->repaint(); }
    
    std::string wnd::get_title() { return native()->get_title(); }
    
    void wnd::set_title(std::string s) { native()->set_title(s); } 
    
    size wnd::get_wsize() { return native()->get_wsize(); }
    
    void wnd::set_wsize(size sz) { native()->set_wsize(sz); } 
    
    pt wnd::get_location() { return native()->get_location(); }
    
    void wnd::set_location(pt location) { native()->set_location(location); } 
    bool app_wnd::on_destroy() {
        // Destroy native window.
        native()->destroy();
        // And tell the world we handled it.
        return true;
    }   

    native_app_wnd* app_wnd::native() {
        if (native_==nullptr) native_=
            std::make_unique<native_app_wnd>(this, title_, size_);
        return native_.get();
    }

    void app_wnd::show() {
        native()->show();
    }
    
    raster::raster(int width, int height) : 
        width_(width), 
        height_(height),
        stride_(width%sizeof(uint8_t)) {
        
        // Calculate raster length.
        len_=stride_ * height;
        // Allocate memory.
        data_=std::make_unique<uint8_t>(len_);
    }

    raster::~raster() {}



#ifdef __WIN__

    void artist::draw_line(color c, pt p1, pt p2) const {
        HPEN pen = ::CreatePen(PS_SOLID, 1, RGB(c.r, c.g, c.b));
        ::SelectObject(canvas_, pen);
        POINT pt;
        ::MoveToEx(canvas_, p1.x, p1.y, &pt);
        ::LineTo(canvas_, p2.x, p2.y);
        ::DeleteObject(pen);
    }

    void artist::draw_rect(color c, rct r) const {
        RECT rect{ r.left, r.top, r.x2(), r.y2() };
        HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
        ::FrameRect(canvas_, &rect, brush);
        ::DeleteObject(brush);
    }

    void artist::fill_rect(color c, rct r) const {   
    }
    native_app_wnd::native_app_wnd(
        app_wnd *window,
        std::string title,
        size size
    ) : native_wnd(window) {

        // Create app window.
        class_ = app::name();

        // Register window.
        ::ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
        wcex_.cbSize = sizeof(WNDCLASSEX);
        wcex_.lpfnWndProc = global_wnd_proc;
        wcex_.hInstance = app::instance();
        wcex_.lpszClassName = class_.c_str();
        wcex_.hCursor = ::LoadCursor(NULL, IDC_ARROW);

        if (!::RegisterClassEx(&wcex_)) 
            throw_ex(nice_exception,"Unable to register class.");

        // Create it.
        hwnd_ = ::CreateWindowEx(
            0,
            class_.c_str(),
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            size.width, size.height,
            NULL,
            NULL,
            app::instance(),
            this);

        if (!hwnd_)
            throw_ex(nice_exception,"Unable to create window.");
    }

    native_app_wnd::~native_app_wnd() {}

    void native_app_wnd::show() const { 
        ::ShowWindow(hwnd_, SW_SHOWNORMAL); 
    }
    void native_wnd::destroy(void) {
        ::PostQuitMessage(0);
    }

    native_wnd::native_wnd(wnd *window) {
        window_=window;
    }

    native_wnd::~native_wnd() {
        ::DestroyWindow(hwnd_);
    }

    void native_wnd::repaint(void) {
         ::InvalidateRect(hwnd_, NULL, TRUE);
    }

    std::string native_wnd::get_title() {
        TCHAR szTitle[1024];
        ::GetWindowTextA(hwnd_, szTitle, 1024);
        return std::string(szTitle);
    }

    void native_wnd::set_title(std::string s) {
        ::SetWindowText(hwnd_,s.c_str());
    }

    size native_wnd::get_wsize() {
        RECT wr;
        ::GetWindowRect(hwnd_, &wr);
        return size{ wr.right-wr.left+1, wr.bottom-wr.top+1 };
    }

    void native_wnd::set_wsize(size sz) {
         // Use move.
        RECT wr;
        ::GetWindowRect(hwnd_, &wr);
        ::MoveWindow(hwnd_, wr.left, wr.top, sz.w, sz.h, TRUE);
    }

    pt native_wnd::get_location() {
        RECT wr;
        ::GetWindowRect(hwnd_, &wr);
        return pt{ wr.left, wr.top };
    }

    void native_wnd::set_location(pt location) {
        // We need to keep the position and just change the size.
        RECT wr;
        ::GetWindowRect(hwnd_, &wr);
        ::MoveWindow(hwnd_, 
            location.left, 
            location.top, 
            wr.right-wr.left+1, 
            wr.bottom-wr.top+1, TRUE);
    }

    LRESULT CALLBACK native_wnd::global_wnd_proc(
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        
        // Is it the very first message? Only on WM_NCCREATE.
        // TODO: Why does Windows 10 send WM_GETMINMAXINFO first?!
        native_wnd* self = nullptr;
        if (message == WM_NCCREATE) {
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            auto self = static_cast<native_wnd*>(lpcs->lpCreateParams);
            self->hwnd_=hWnd; // save the window handle too!
            ::SetWindowLongPtr(
                hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
            self = reinterpret_cast<native_wnd*>
                (::GetWindowLongPtr(hWnd, GWLP_USERDATA));

        // Chain...
        if (self != nullptr)
            return (self->local_wnd_proc(message, wParam, lParam));
        else
            return ::DefWindowProc(hWnd, message, wParam, lParam);
    }
    
    LRESULT native_wnd::local_wnd_proc(
        UINT msg, WPARAM wparam, LPARAM lparam) {

        switch (msg) {
            case WM_CREATE:
                window_->created.emit();
                break;
            case WM_DESTROY:
                window_->destroyed.emit();
                break;
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd_, &ps);
                artist a(hdc);
                window_->paint.emit(a);
                EndPaint(hwnd_, &ps);
            }
            break;
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
            {
                // Populate the mouse info structure.
                mouse_info mi = {
                    {GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam)}, // point
                    (bool)(wparam & MK_LBUTTON),
                    (bool)(wparam & MK_MBUTTON),
                    (bool)(wparam & MK_RBUTTON),
                    (bool)(wparam & MK_CONTROL),
                    (bool)(wparam & MK_SHIFT)
                };
                if (msg == WM_MOUSEMOVE)
                    window_->mouse_move.emit(mi);
                else if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN)
                    window_->mouse_down.emit(mi);
                else
                    window_->mouse_up.emit(mi);
            }
            break;
            case WM_SIZE:
            {
                rct r = rct{ 0, 0, LOWORD(lparam), HIWORD(lparam) };
                window_->resized.emit(
                    {
                        LOWORD(lparam),
                        HIWORD(lparam)
                    }
                );
            }
                break;
            default:
                return ::DefWindowProc(hwnd_, msg, wparam, lparam);
            }
            return 0;

    }
    app_id app::id() {
        return ::GetCurrentProcessId();
    }

    bool app::is_primary_instance() {
        // Are we already primary instance? If not, try to become one.
        if (!primary_) {
            std::string aname = app::name();
            // Create local mutex.
            std::ostringstream name;
            name << "Local\\" << aname;
            ::CreateMutex(0, FALSE, name.str().c_str());
            // We are primary instance.
            primary_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
        }
        return primary_;
    }

    void app::run(const app_wnd& w) {

        // We have to cast the constness away to 
        // call non-const functions on window.
        auto& main_wnd=const_cast<app_wnd &>(w);
        main_wnd.show();

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

#elif __X11__

    void artist::draw_line(color c, pt p1, pt p2) const {
        
    }

    void artist::draw_rect(color c, rct r) const {
        
    }

    void artist::fill_rect(color c, rct r) const {   
    }
    native_app_wnd::native_app_wnd(
        app_wnd *window,
        std::string title,
        size size
    ) : native_wnd(window) {

        int s = DefaultScreen(display_);
        winst_ = ::XCreateSimpleWindow(
            display_, 
            RootWindow(display_, s), 
            10, // x 
            10, // y
            size.width, 
            size.height, 
            1, // border width
            BlackPixel(display_, s), // border color
            WhitePixel(display_, s)  // background color
        );
        // Store window to window list.
        wmap_.insert(std::pair<Window,native_wnd*>(winst_, this));
        // Set initial title.
        ::XSetStandardProperties(display_,winst_,title.c_str(),NULL,None,NULL,0,NULL);

        // Rather strange handling of close window by X11.
        Atom atom = XInternAtom ( display_,"WM_DELETE_WINDOW", false );
        ::XSetWMProtocols(display_, winst_, &atom, 1);

        // TODO: Implement lazy subscription (somday)
        ::XSelectInput (display_, winst_,
			ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | 
            LeaveWindowMask | PointerMotionMask | FocusChangeMask | KeyPressMask |
            KeyReleaseMask | SubstructureNotifyMask | StructureNotifyMask | 
            SubstructureRedirectMask);
    }

    native_app_wnd::~native_app_wnd() {}

    void native_app_wnd::show() const { 
        ::XMapWindow(display_, winst_);
    }
    // Static variable.
    std::map<Window,native_wnd*> native_wnd::wmap_;

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
        char * name;
        ::XFetchName(display_,winst_, &name);
        std::string wname=name;
        ::XFree(name);
        return wname;
    }

    size native_wnd::get_wsize() {
        XWindowAttributes wattr;
        ::XGetWindowAttributes(display_,winst_,&wattr);
        // TODO: I think this is just the client area?
        return { wattr.width, wattr.height };
    }

    void native_wnd::set_wsize(size sz) {
        XResizeWindow(display_,winst_, sz.w, sz.h);
    }

    pt native_wnd::get_location() {
        XWindowAttributes wattr;
        ::XGetWindowAttributes(display_,winst_,&wattr);
        // TODO: I think this is just the client area?
        return { wattr.x, wattr.y };
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
                (bool)(e.xbutton.button&Button1), 
                (bool)(e.xbutton.button&Button2),
                (bool)(e.xbutton.button&Button3),
                (bool)(e.xbutton.state&ControlMask),
                (bool)(e.xbutton.state&ShiftMask)
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
                (bool)(e.xmotion.state&Button1Mask), 
                (bool)(e.xmotion.state&Button2Mask),
                (bool)(e.xmotion.state&Button3Mask),
                (bool)(e.xmotion.state&ControlMask),
                (bool)(e.xmotion.state&ShiftMask)
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
    app_id app::id() {
        return ::getpid();
    }

    bool app::is_primary_instance() {
        // Are we already primary instance? If not, try to become one.
        if (!primary_) {
            std::string aname = app::name();

            // Pid file needs to go to /var/run
            std::ostringstream pfname, pid;
            pfname << "/tmp/" << aname << ".pid";
            pid << nice::app::id() << std::endl;

            // Open, lock, and forget. Let the OS close and unlock.
            int pfd = ::open(pfname.str().c_str(), O_CREAT | O_RDWR, 0666);
            int rc = ::flock(pfd, LOCK_EX | LOCK_NB);
            primary_ = !(rc && EWOULDBLOCK == errno);
            if (primary_) {
                // Write our process id into the file.
                ::write(pfd, pid.str().c_str(), pid.str().length());
                return false;
            }
        }
        return primary_;
    }

    void app::run(const app_wnd& w) {

        // We have to cast the constness away to 
        // call non-const functions on window.
        auto& main_wnd=const_cast<app_wnd &>(w);

        // Show the window.
        main_wnd.show();

        // Flush it all.
        ::XFlush(instance_.display);

        // Main event loop.
        XEvent e;
        bool quit=false;
	    while ( !quit ) // Will be interrupted by the OS.
	    {
	      ::XNextEvent ( instance_.display,&e );
	      quit = native_wnd::global_wnd_proc(e);
	    }
    }

#elif __SDL__

    void artist::draw_line(color c, pt p1, pt p2) const {
        
    }

    void artist::draw_rect(color c, rct r) const {
        
    }

    void artist::fill_rect(color c, rct r) const {   
    }
    native_app_wnd::native_app_wnd(
        app_wnd *window,
        std::string title,
        size size
    ) : native_wnd(window) {
        /* Create window. In screen coordinates. */
        winst_ = ::SDL_CreateWindow(title.c_str(), 
            SDL_WINDOWPOS_UNDEFINED, 
            SDL_WINDOWPOS_UNDEFINED, 
            size.w, 
            size.h, 
            SDL_WINDOW_HIDDEN);

        // Store window to window list.
        wmap_.insert(std::pair<SDL_Window*,native_wnd*>(winst_, this));
    }

    native_app_wnd::~native_app_wnd() {}

    void native_app_wnd::show() const { 
        ::SDL_ShowWindow(winst_);
    }
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
    app_id app::id() {
        return ::getpid();
    }

    bool app::is_primary_instance() {
        // Are we already primary instance? If not, try to become one.
        if (!primary_) {
            std::string aname = app::name();

            // Pid file needs to go to /var/run
            std::ostringstream pfname, pid;
            pfname << "/tmp/" << aname << ".pid";
            pid << nice::app::id() << std::endl;

            // Open, lock, and forget. Let the OS close and unlock.
            int pfd = ::open(pfname.str().c_str(), O_CREAT | O_RDWR, 0666);
            int rc = ::flock(pfd, LOCK_EX | LOCK_NB);
            primary_ = !(rc && EWOULDBLOCK == errno);
            if (primary_) {
                // Write our process id into the file.
                ::write(pfd, pid.str().c_str(), pid.str().length());
                return false;
            }
        }
        return primary_;
    }

    void app::run(const app_wnd& w) {

        // Show the main window.
        auto& main_wnd=const_cast<app_wnd &>(w);
        main_wnd.show();


        /*
        w = SDL_CreateWindow("zwin simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_HIDDEN);
        if (w == NULL)
            printf("window could not be created: %s\n", SDL_GetError());
        else
        {
            SDL_SetWindowSize(w, SCREEN_WIDTH, SCREEN_HEIGHT);
            SDL_ShowWindow(w);
        */

        // Main event loop.
        SDL_Event e;
        /* Clean the queue */
        bool quit=false;
        while (!quit) { 
            // Wait for something to happen.
            SDL_WaitEvent(&e);
            /* User requested quit? */
            if (e.type == SDL_QUIT)
                quit = true;
        }

    }

#endif

}

#ifdef __WIN__
extern void program();

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Store cmd line arguments to vector.
    int argc = __argc;
    char** argv = __argv;
    nice::app::instance(hInstance);

    // Copy cmd line arguments to vector.
    nice::app::args = std::vector<std::string>(argv, argv + argc);

    // Try becoming primary instance...
    nice::app::is_primary_instance();
    
    // Run program.
    program();
    
    // And return return code;
    return nice::app::ret_code;
}

#elif __X11__
extern void program();

int main(int argc, char* argv[]) {
    // X Windows initialization code.
    nice::app_instance inst;
    inst.display=::XOpenDisplay(NULL);
    nice::app::instance(inst);

    // Copy cmd line arguments to vector.
    nice::app::args = std::vector<std::string>(argv, argv + argc);

    // Try becoming primary instance...
    nice::app::is_primary_instance();
    
    // Run program.
    program();
    
    // Close display.
    ::XCloseDisplay(inst.display);

    // And return return code;
    return nice::app::ret_code;
}

#elif __SDL__
extern void program();

int main(int argc, char* argv[]) {

    // Init SDL.
    if (::SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw_ex(nice::nice_exception,"SDL could not initialize.");
    }

    // TODO: Initialize application.
    nice::app_instance inst=0;
    nice::app::instance(inst);

    // Copy cmd line arguments to vector.
    nice::app::args = std::vector<std::string>(argv, argv + argc);

    // Try becoming primary instance...
    nice::app::is_primary_instance();
    
    // Run program.
    program();
    
    // Exit SDL.
    ::SDL_Quit();

    // And return return code;
    return nice::app::ret_code;
}

#endif

#endif // _NICE_HPP
