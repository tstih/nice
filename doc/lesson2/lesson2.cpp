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

extern void program();

namespace ni {

#if _WIN32
    typedef DWORD  app_id;
    typedef HINSTANCE app_instance;
    typedef HWND wnd_instance;
    typedef LONG coord;
#elif __unix__ 
    typedef pid_t app_id;
    typedef GtkApplication* app_instance;
    typedef GtkWidget* wnd_instance;
    typedef int coord;
#endif


    struct size {
        union { coord width; coord w; };
        union { coord height; coord h; };
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
            return !(instance_==nullptr);
        }

    private:
        // Store resource value here.
        mutable T instance_{ nullptr };
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
    };

    class app_wnd : public wnd {
    public:
        app_wnd(std::string title, size size) : wnd() {
            title_=title;
            size_=size;
        }
        wnd_instance create() override;
    private:
        std::string title_;
        size size_;
#if WIN32
        WNDCLASSEX wcex_;
        std::string class_;
        static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
            switch (message) {
            case WM_DESTROY:
                ::PostQuitMessage(0);
                break;
            }
            // If we are here, do the default stuff.
            return ::DefWindowProc(hWnd, message, wParam, lParam);
        }
#endif
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
        static app_instance instance_;

#if _WIN32
        friend int WINAPI ::WinMain(
            _In_ HINSTANCE hInstance,
            _In_opt_ HINSTANCE hPrevInstance,
            _In_ LPSTR lpCmdLine,
            _In_ int nShowCmd);
#elif __unix__
        friend int main(int argc, char* argv[]);
        struct wnd2void{const app_wnd& w;};
        static void gtk_app_activate (GtkApplication *app, gpointer user_data);
#endif
    };

    int app::ret_code = 0;
    std::vector<std::string> app::args;

    bool app::primary_ = false;
    app_instance app::instance_ = nullptr;

    std::string app::name() {
        return std::filesystem::path(args[0]).stem().string();
    }

    app_instance app::instance() {
        return instance_;
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

#if __unix__ 
    void app::gtk_app_activate(GtkApplication *app, gpointer user_data) {
        wnd2void *wv2=static_cast<wnd2void*>(user_data); // Get window structure.
        gtk_widget_show_all ((wv2->w).instance());
    }
#endif

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
        // Create application instance.
        char *app_name=name().c_str();
        instance_ = ::gtk_application_new(NULL,G_APPLICATION_FLAGS_NONE);
        // Connect activate.
        wnd2void w2v { w };
        g_signal_connect (instance_, "activate", G_CALLBACK (app::gtk_app_activate), &w2v);
        // Message loop.
        ret_code = ::g_application_run(G_APPLICATION(instance_), 0, NULL);
        // Cleanup.
        ::g_object_unref(instance_);
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
        wnd_instance w = gtk_application_window_new (app::instance());
        gtk_window_set_title (GTK_WINDOW (w), title_.c_str());
        gtk_window_set_default_size (GTK_WINDOW (w), size_.width, size_.height);
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

    ni::app::instance_ = hInstance;

#elif __unix__
int main(int argc, char* argv[]) {
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

using namespace ni;

void program() {
    app::run(
        app_wnd("Hello World!", { 640, 400 })
    );
}