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
    typedef HWND wnd_id;
    typedef LONG coord;
#elif __unix__ 
    typedef pid_t app_id;
    typedef GtkWidget* wnd_id;
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
        virtual void id(T id) { id_ = id; }

        // Id getter with lazy eval.
        virtual T id(bool eval_only=false) const {
            // Lazy evaluate by callign create.
            if (!eval_only)
                if (id_ == nullptr)
                    id(create());
            // Return.
            return id_;
        }

    private:
        // Store resource value here.
        mutable T id_{ nullptr };
    };

    class wnd : public resource<wnd_id> {
    public:
        virtual wnd_id create() = 0;
        void destroy() noexcept override {
#if _WIN32
            ::DestroyWindow(id(true)); id(nullptr);
#elif __unix__ 

#endif
        }
    };

    class app_wnd : public wnd {
    public:
        app_wnd(std::string title, size size) {
        }

        wnd_id create() override { return nullptr; }
    };

    class app {
    public:
        // Cmd line arguments.
        static std::vector<std::string> args;

        // Return code.
        static int ret_code;

        // Process id.
        static app_id id();

        // Application name.
        static std::string name();

        // Is another instance already running?
        static bool is_primary_instance();

        // Main desktop application loop.
        static void run(const app_wnd& w);

    private:
        static app_id id_;
        static bool pinst_;

#if _WIN32
#elif __unix__ 
        static GtkApplication* gtk_app_;
        static void gtk_app_activate(GtkApplication* app, gpointer user_data);
#endif
    };

    int app::ret_code = 0;
    bool app::pinst_ = false;



    std::vector<std::string> app::args;

#if _WIN32
#elif __unix__ 
    GtkApplication* app::gtk_app_ = nullptr;
    void app::gtk_app_activate(GtkApplication* app, gpointer user_data) {

    }
#endif



    std::string app::name() {
        return std::filesystem::path(args[0]).stem().string();
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
        if (!pinst_) {
            std::string aname = name();
#if _WIN32
            // Create local mutex.
            std::ostringstream name;
            name << "Local\\" << aname;
            ::CreateMutex(0, FALSE, name.str().data());
            // We are primary instance.
            pinst_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
#elif __unix__
            // Pid file needs to go to /var/run
            std::ostringstream pfname, pid;
            pfname << "/tmp/" << aname << ".pid";
            pid << ni::app::id() << std::endl;

            // Open, lock, and forget. Let the OS close and unlock.
            int pfd = ::open(pfname.str().data(), O_CREAT | O_RDWR, 0666);
            int rc = ::flock(pfd, LOCK_EX | LOCK_NB);
            pinst_ = !(rc && EWOULDBLOCK == errno);
            if (pinst_) {
                // Write our process id into the file.
                ::write(pfd, pid.str().data(), pid.str().length());
                return false;
            }
#endif
        }
        return pinst_;
    }

    void app::run(const app_wnd& w) {
#if _WIN32
#elif __unix__ 
        gtk_app_ = ::gtk_application_new(name().data(),G_APPLICATION_FLAGS_NONE);
        ::g_signal_connect(gtk_app_, "activate", G_CALLBACK(gtk_app_activate), NULL);
        ret_code = ::g_application_run(G_APPLICATION(gtk_app_), 0, NULL);
        ::g_object_unref(gtk_app_);
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
    // Your code here.
}