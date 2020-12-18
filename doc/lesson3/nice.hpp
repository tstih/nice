#ifndef _NICE_HPP
#define _NICE_HPP

extern "C" {
#if _WIN32
#include <windows.h>
#elif __unix__
#include <sys/file.h>
#include <unistd.h>
#include <gtk/gtk.h>
#endif
}
#include <vector>
#include <filesystem>
#include <sstream>
#include <map>
#include <functional>

extern void program();

namespace ni {

#if _WIN32
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
#elif __unix__ 
    typedef pid_t app_id;
    typedef GtkApplication* app_instance; // Not used.
    typedef GtkWidget* wnd_instance;
    typedef int coord;
    typedef uint8_t byte;
    typedef cairo_t* canvas;
#endif


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


    template<typename T>
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
            if (instance_ == nullptr)
                instance_ = const_cast<resource<T>*>(this)->create();
            // Return.
            return instance_;
        }

        bool initialized() {
            return !(instance_ == nullptr);
        }

    private:
        // Store resource value here.
        mutable T instance_{ nullptr };
    };


    class artist {
    public:
        // Pass canvas instance, don't own it.
        artist(const canvas& canvas) {
            canvas_ = canvas;
        }

        // Draw a rectangle.
        void draw_rect(color c, rct r) const {
#if _WIN32
            RECT rect{ r.left, r.top, r.x2(), r.y2() };
            HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
            ::FrameRect(canvas_, &rect, brush);
            ::DeleteObject(brush);
#elif __unix__
            cairo_set_source_rgb(canvas_, ((float)c.r)/255.0f, ((float)c.g)/255.0f, ((float)c.b)/255.0f);
            cairo_set_line_width(canvas_, 1);
            cairo_rectangle(canvas_, r.left + 0.5f, r.top + 0.5F, r.width, r.height );
            cairo_stroke_preserve(canvas_);
#endif
        }
    private:
        canvas canvas_;
    };

    class wnd : public resource<wnd_instance> {
    public:
        virtual ~wnd() { destroy(); }
        virtual wnd_instance create() = 0;
        void destroy() noexcept override {
#if _WIN32
            if (initialized())
                ::DestroyWindow(instance()); instance(nullptr);
#endif
        }

        // Events of basic window.
#if _WIN32
        signal<> created;
        signal<> destroyed;
        signal<const artist&> paint;
#elif __unix__
        signal<> created;
        signal<> destroyed{ [this]() { ::g_signal_connect(G_OBJECT(instance()), "destroy", G_CALLBACK(wnd::global_gtk_destroy), this); } };
        signal<const artist&> paint{ [this]() { ::g_signal_connect(G_OBJECT(instance()), "draw", G_CALLBACK(wnd::global_gtk_draw), this); } };
#endif
    protected:
#if _WIN32
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
#elif __unix__
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
#endif
    };

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
#if WIN32
        WNDCLASSEX wcex_;
        std::string class_;
#endif
    protected:
        bool on_destroy() {
#if _WIN32
            ::PostQuitMessage(0);
#elif __unix__
            ::gtk_main_quit();
#endif
            return true; // Message processed.
        }            
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

        // Application instance.
        static app_instance instance();

        // Is another instance already running?
        static bool is_primary_instance();

        // Main desktop application loop.
        static void run(const app_wnd& w);

    private:
        static bool primary_;
    };

    int app::ret_code = 0;
    std::vector<std::string> app::args;

    bool app::primary_ = false;

    std::string app::name() {
        return std::filesystem::path(args[0]).stem().string();
    }

    app_instance app::instance() {
#if _WIN32
        return ::GetModuleHandle(NULL);
#elif __unix__
        return nullptr;
#endif
    }

    app_id app::id() {
#if _WIN32
        return ::GetCurrentProcessId();
#elif __unix__
        return ::getpid();
#endif
    }

    bool app::is_primary_instance() {
        // Are we already primary instance? If not, try to become one.
        if (!primary_) {
            std::string aname = app::name();
#if _WIN32
            // Create local mutex.
            std::ostringstream name;
            name << "Local\\" << aname;
            ::CreateMutex(0, FALSE, name.str().c_str());
            // We are primary instance.
            primary_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
#elif __unix__
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

    void app::run(const app_wnd& w) {
#if _WIN32
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

#elif __unix__ 
        // Show window. Will lazy evaluate window.
        gtk_widget_show(w.instance());

        // Message loop.
        gtk_main();
#endif
    }


    wnd_instance app_wnd::create() {
#if _WIN32
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
#elif __unix__ 
        // Create it.
        wnd_instance w = ::gtk_window_new(GTK_WINDOW_TOPLEVEL);

        // Make sure it is paintable.
        ::gtk_widget_set_app_paintable(w,TRUE);

        // Set title and height.
        ::gtk_window_set_title(GTK_WINDOW(w), title_.c_str());
        ::gtk_window_set_default_size(GTK_WINDOW(w), size_.width, size_.height);
        return w;
#endif
    }
}

#if _WIN32
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Store cmd line arguments to vector.
    int argc = __argc;
    char** argv = __argv;
#elif __unix__
int main(int argc, char* argv[]) {
    // Initialize GTK (classic init, without GtkApplication, don't process cmd line).
    ::gtk_init(NULL, NULL);
#endif
    // Copy cmd line arguments to vector.
    ni::app::args = std::vector<std::string>(argv, argv + argc);

    // Try becoming primary instance...
    ni::app::is_primary_instance();
    
    // Run program.
    program();
    
    // And return return code;
    return ni::app::ret_code;
}

#endif // _NICE_HPP