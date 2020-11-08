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

extern void program();

namespace ni {

#if _WIN32
    typedef DWORD  app_id;
#elif __unix__ 
    typedef pid_t app_id;
#endif

    class app {
    public:
        // Cmd line arguments.
        static std::vector<std::string> args;

        // Return code.
        static int ret_code;

        // Process id.
        static app_id id();

        // Is another instance already running?
        static bool is_already_running();

    private:
        static app_id id_;
    };

    int app::ret_code = 0;
    std::vector<std::string> app::args;

    app_id app::id() {
#if _WIN32
        return ::GetCurrentProcessId();
#elif __unix__
        return ::getpid();
#endif
    }
    bool app::is_already_running() {
        std::string aname = std::filesystem::path(args[0]).stem();
        std::ostringstream name;
#if _WIN32
        // Create local mutex.
        name << "Local\\" << aname;
        ::CreateMutex(0, FALSE, name.str().data());
        return (::GetLastError() == ERROR_ALREADY_EXISTS);
#elif __unix__
        // Pid file needs to go to /var/run
        std::ostringstream pfname, pid;
        pfname << "/tmp/" << aname << ".pid";
        pid << ni::app::id() << std::endl;

        // Open, lock, and forget. Let the OS close and unlock.
        int pfd = ::open(pfname.str().data(), O_CREAT | O_RDWR, 0666);
        int rc = ::flock(pfd, LOCK_EX | LOCK_NB);
        if (rc && EWOULDBLOCK == errno) return true;
        else {
            // Write our process id into the file.
            ::write(pfd, pid.str().data(), pid.str().length());
            return false;
        }
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

    // Run program.
    program();

    // And return return code;
    return ni::app::ret_code;
}

using namespace ni;

void program() {
    // Your code here.
}