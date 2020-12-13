extern "C" {
#if _WIN32
#include <windows.h>
#elif __unix__
#include <sys/file.h>
#include <unistd.h>
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
#elif __unix__ 
    typedef pid_t app_id;
    typedef void* app_instance;
#endif

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
            ::CreateMutex(0, FALSE, name.str().data());
            // We are primary instance.
            primary_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
#elif __unix__
            // Pid file needs to go to /var/run
            std::ostringstream pfname, pid;
            pfname << "/tmp/" << aname << ".pid";
            pid << ni::app::id() << std::endl;

            // Open, lock, and forget. Let the OS close and unlock.
            int pfd = ::open(pfname.str().data(), O_CREAT | O_RDWR, 0666);
            int rc = ::flock(pfd, LOCK_EX | LOCK_NB);
            primary_ = !(rc && EWOULDBLOCK == errno);
            if (primary_) {
                // Write our process id into the file.
                ::write(pfd, pid.str().data(), pid.str().length());
                return false;
            }
#endif
        }
        return primary_;
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
    // Your code here.
}