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

extern "C" {
#if __WIN__
#include <windows.h>
#elif __GTK__
#include <sys/file.h>
#include <unistd.h>
#include <gtk/gtk.h>
extern int main(int argc, char* argv[]);
#elif __X11__
#include <sys/file.h>
#include <unistd.h>
#include <X11/Xlib.h>

#endif
}
#include <vector>
#include <filesystem>
#include <sstream>
#include <map>
#include <functional>

extern void program();

#if __GTK__|| __X11__
extern "C" {
extern int main(int argc, char* argv[]);
} 
#endif

namespace ni {

// ----- Unified types. -------------------------------------------------------
#if __WIN__
    typedef DWORD  app_id;
    typedef HINSTANCE app_instance;
    typedef HWND wnd_instance;
    typedef LONG coord;
    typedef UINT msg;
    typedef WPARAM par1;
    typedef LPARAM par2;
    typedef LRESULT result;
    typedef BYTE byte;
    typedef HDC canvas;
#elif __GTK__ 
    typedef pid_t app_id;
    typedef GtkApplication* app_instance; // Not used.
    typedef GtkWidget* wnd_instance;
    typedef int coord;
    typedef uint8_t byte;
    typedef cairo_t* canvas;
#elif __X11__
    typedef pid_t app_id;
    typedef Display* app_instance;
    typedef Window wnd_instance;
    typedef int coord;
    typedef uint8_t byte;
    typedef GC canvas;
#endif



// ----- Unified structures. --------------------------------------------------
    struct size {
        union { coord width; coord w; };
        union { coord height; coord h; };
    };

    struct color {
        byte r;
        byte g;
        byte b;
        byte a;
    };

    struct pt {
        union { coord left; coord x; };
        union { coord top; coord y; };
    };

    struct rct {
        union { coord left; coord x; coord x1; };
        union { coord top; coord y;  coord y1; };
        union { coord width; coord w; };
        union { coord height; coord h; };
        coord x2() { return left + width; }
        coord y2() { return top + height; }
    };



 // ----- Signals. ------------------------------------------------------------
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



// ----- Two phase construction. ----------------------------------------------
    template<typename T, T N=nullptr>
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
                instance_ = const_cast<resource<T,N>*>(this)->create();
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



// ----- Graphics & painting. -------------------------------------------------
    class artist {
    public:
        // Pass canvas instance, don't own it.
        artist(const canvas& canvas) {
            canvas_ = canvas;
        }

        // Draw a rectangle.
        void draw_rect(color c, rct r) const {
#if __WIN__
            RECT rect{ r.left, r.top, r.x2(), r.y2() };
            HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
            ::FrameRect(canvas_, &rect, brush);
            ::DeleteObject(brush);
#elif __GTK__
            cairo_set_source_rgb(canvas_, normalize(c.r), normalize(c.g), normalize(c.b));
            cairo_set_line_width(canvas_, 1);
            cairo_rectangle(canvas_, r.left + 0.5f, r.top + 0.5F, r.width, r.height );
            cairo_stroke(canvas_);
#endif
        }

        // Draw a rectangle.
        void fill_rect(color c, rct r) const {
#if __WIN__
#elif __GTK__
            cairo_set_source_rgb(canvas_, normalize(c.r), normalize(c.g), normalize(c.b));
            cairo_set_line_width(canvas_, 1);
            cairo_rectangle(canvas_, r.left + 0.5f, r.top + 0.5F, r.width, r.height );
            cairo_fill(canvas_);
#endif
        }
    private:
        canvas canvas_;
#ifdef __GTK__
        float normalize(int c, int max=255) const {
            return (float)c / (float) max;
        }
#endif
    };



// ----- Base window.  --------------------------------------------------------
#if __X11__
    class wnd : public resource<wnd_instance, 0> {
#else
    class wnd : public resource<wnd_instance> {
#endif
    public:
        virtual ~wnd() { destroy(); }
        virtual wnd_instance create() = 0;
        void destroy() noexcept override;

        // Events of basic window.
#if __WIN__
        signal<> created;
        signal<> destroyed;
        signal<const artist&> paint;
#elif __GTK__
        signal<> created;
        signal<> destroyed{ [this]() { ::g_signal_connect(G_OBJECT(instance()), "destroy", G_CALLBACK(wnd::global_gtk_destroy), this); } };
        signal<const artist&> paint{ [this]() { ::g_signal_connect(G_OBJECT(instance()), "draw", G_CALLBACK(wnd::global_gtk_draw), this); } };
#elif __X11__
        signal<> created;
        signal<> destroyed;
        signal<const artist&> paint;
#endif
    protected:
#if __WIN__
        // Generic callback. Calls member callback.
        static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            // Is it the very first message? Only on WM_NCCREATE.
            // TODO: Why does Windows 10 send WM_GETMINMAXINFO first?!
            wnd* self = nullptr;
            if (message == WM_NCCREATE) {
                LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
                auto self = static_cast<wnd*>(lpcs->lpCreateParams);
                self->instance(hWnd); // save the window handle too!
                ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else
                self = reinterpret_cast<wnd*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

            // Chain...
            if (self != nullptr)
                return (self->local_wnd_proc(message, wParam, lParam));
            else
                return ::DefWindowProc(hWnd, message, wParam, lParam);
        }

        // Local callback. Specific to wnd.
        virtual result local_wnd_proc(msg msg, par1 p1, par2 p2) {
            switch (msg) {
            case WM_CREATE:
                created.emit();
                break;
            case WM_DESTROY:
                destroyed.emit();
                break;
            case WM_PAINT: // New paint handler!
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(instance(), &ps);
                artist a(hdc);
                paint.emit(a);
                EndPaint(instance(), &ps);
            }
            break;
            default:
                return ::DefWindowProc(instance(), msg, p1, p2);
            }
            return 0;
        }
#elif __GTK__
        static void global_gtk_destroy(GtkWidget* widget, gpointer data) {
            wnd* w = reinterpret_cast<wnd*>(data);
            w->destroyed.emit();
        }
        static gboolean global_gtk_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
        {
            wnd* w = reinterpret_cast<wnd*>(data);
            artist a(cr);
            w->paint.emit(a);
            return FALSE;
        }
#elif __X11__
        friend void global_handle_x11_event(const XEvent& e);
        void local_handle_x11_event(const XEvent& e) {

        }
#endif
    };



// ----- Application window. --------------------------------------------------
    class app_wnd : public wnd {
    public:
        app_wnd(std::string title, size size) : wnd() {
            // Store parameters.
            title_ = title; size_ = size;
            // Subscribe to destroy signal.
            destroyed.connect(this, &app_wnd::on_destroy);
        }
        wnd_instance create() override;
    private:
        std::string title_;
        size size_;
#if __WIN__
        WNDCLASSEX wcex_;
        std::string class_;
#endif
    protected:
        bool on_destroy() {
#if __WIN__
            ::PostQuitMessage(0);
#elif __GTK__
            ::gtk_main_quit();
#endif
            return true; // Message processed.
        }            
    };



// ----- Application. ---------------------------------------------------------
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

        // Startup functions are our friends.
#if __WIN__
        friend int WINAPI WinMain(
            _In_ HINSTANCE hInstance, 
            _In_opt_ HINSTANCE hPrevInstance, 
            _In_ LPSTR lpCmdLine, 
            _In_ int nShowCmd);
#elif __GTK__ || __X11__
        friend int ::main(int argc, char* argv[]);
#endif
        
    };

    int app::ret_code = 0;
    std::vector<std::string> app::args;

    bool app::primary_ = false;
    app_instance app::instance_;

    std::string app::name() {
        return std::filesystem::path(args[0]).stem().string();
    }

    app_instance app::instance() { return instance_; }

    app_id app::id() {
#if __WIN__
        return ::GetCurrentProcessId();
#elif __GTK__
        return ::getpid();
#endif
    }

    bool app::is_primary_instance() {
        // Are we already primary instance? If not, try to become one.
        if (!primary_) {
            std::string aname = app::name();
#if __WIN__
            // Create local mutex.
            std::ostringstream name;
            name << "Local\\" << aname;
            ::CreateMutex(0, FALSE, name.str().c_str());
            // We are primary instance.
            primary_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
#elif __GTK__ || __X11__
            // Pid file needs to go to /var/run
            std::ostringstream pfname, pid;
            pfname << "/tmp/" << aname << ".pid";
            pid << ni::app::id() << std::endl;

            // Open, lock, and forget. Let the OS close and unlock.
            int pfd = ::open(pfname.str().c_str(), O_CREAT | O_RDWR, 0666);
            int rc = ::flock(pfd, LOCK_EX | LOCK_NB);
            primary_ = !(rc && EWOULDBLOCK == errno);
            if (primary_) {
                // Write our process id into the file.
                ::write(pfd, pid.str().c_str(), pid.str().length());
                return false;
            }
#endif
        }
        return primary_;
    }

#if __X11__
    std::map<wnd_instance,wnd*> wmap;
    void global_handle_x11_event(const XEvent& e) {
        wnd* w = (wnd*)e.xany.window;
        w->local_handle_x11_event(e);
    }
#endif

    void app::run(const app_wnd& w) {
#if __WIN__
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

#elif __GTK__ 
        // Show window. Will lazy evaluate window.
        gtk_widget_show(w.instance());

        // Message loop.
        gtk_main();

#elif __X11__

        ::XMapWindow(app::instance(), w.instance());
        ::XFlush(app::instance());

        XEvent e;
	    while ( true ) // Will be interrupted by the OS.
	    {
	      ::XNextEvent ( app::instance(),&e );
	      global_handle_x11_event(e);
	    }
#endif
    }



// ----- Deferred definitions (misc.) ----------------------------------------- 

    void wnd::destroy() noexcept {
#if __WIN__
        if (initialized()) {
            ::DestroyWindow(instance()); instance(nullptr);
        }
#elif __X11__
        if (initialized()) {
            // Remove from windows map.
            wmap.erase (instance()); 
            // And destroy.
            XDestroyWindow(app::instance(), instance()); instance(0);
        } 
#endif
    }

    wnd_instance app_wnd::create() {
#if __WIN__
        // Get class.
        class_ = app::name();

        // Register window.
        ::ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
        wcex_.cbSize = sizeof(WNDCLASSEX);
        wcex_.lpfnWndProc = global_wnd_proc;
        wcex_.hInstance = app::instance();
        wcex_.lpszClassName = class_.c_str();
        wcex_.hCursor = ::LoadCursor(NULL, IDC_ARROW);

        if (!::RegisterClassEx(&wcex_)) nullptr; // TODO: Throw an error.

        // Create it.
        HWND hwnd = ::CreateWindowEx(
            0,
            class_.c_str(),
            title_.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            size_.width, size_.height,
            NULL,
            NULL,
            app::instance(),
            this);

        if (!hwnd) return nullptr; // TODO: Throw an error.

        return hwnd;
#elif __GTK__ 
        // Create it.
        wnd_instance w = ::gtk_window_new(GTK_WINDOW_TOPLEVEL);

        // Make sure it is paintable.
        ::gtk_widget_set_app_paintable(w,TRUE);

        // Set title and height.
        ::gtk_window_set_title(GTK_WINDOW(w), title_.c_str());
        ::gtk_window_set_default_size(GTK_WINDOW(w), size_.width, size_.height);
        return w;
#elif __X11__
        int s = DefaultScreen(app::instance());
        Window w = ::XCreateSimpleWindow(
            app::instance(), 
            RootWindow(app::instance(), s), 
            10, // x 
            10, // y
            size_.width, 
            size_.height, 
            1, // border width
            BlackPixel(app::instance(), s), // border color
            WhitePixel(app::instance(), s)  // background color
        );
        // Store window to window list.
        wmap.insert(std::pair<Window,wnd*>(w,this));
        return w;
#endif
    }

} // namespace



// ----- Application entry point. ---------------------------------------------
#if __WIN__
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Store cmd line arguments to vector.
    int argc = __argc;
    char** argv = __argv;
    ni::app::instance_=hInstance;
#elif __GTK__
int main(int argc, char* argv[]) {
    // Initialize GTK (classic init, without GtkApplication, don't process cmd line).
    ::gtk_init(NULL, NULL);
    ni::app::instance_=nullptr; // TODO: One day GtkApplication.
#elif __X11__
int main(int argc, char* argv[]) {
    // X Windows initialization code.
    ni::app::instance_=XOpenDisplay(NULL);
#endif
    // Copy cmd line arguments to vector.
    ni::app::args = std::vector<std::string>(argv, argv + argc);

    // Try becoming primary instance...
    ni::app::is_primary_instance();
    
    // Run program.
    program();
    
#if __X11__
    // Close display.
    XCloseDisplay(ni::app::instance());
#endif

    // And return return code;
    return ni::app::ret_code;
}

#endif // _NICE_HPP