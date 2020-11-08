# The Desktop Application

When the operating system yields control to our application, 
it executes some bootstrap code and then jumps to your 
applications' entry point. In a traditional C/C++ application, 
this entry point is a function called `main()`.

Main accepts command line arguments, and returns an integer. 
This integer is called the return code and is passed back
to the operating system. A zero return value signals success 
and any other value could indicate an error.

Some operating systems don't follow this convention. For example, 
MS Windows main entry point of a desktop application is a function
called `WinMain()`. 

Having different entry points on each platform would defeat our 
purpose of being nice. Recall, that we want to write code like this

~~~cpp
#include "nice.hpp"

using namespace ni;

void program()
{
    app::run(app_wnd("Hello World!"));
}
~~~

Obviously this will not work out of the box hence our library needs 
to provide some glue for that. The first task for our framework is to 
rewire the platform specific application entry point to unified (multiplatofrm) 
application entry point.

## Nice Application

Lets start this process by declaring an standard entry point function called `program()`
and a basic application class for storing command line arguments, and return code.

~~~cpp
#include <string>
#include <vector>

extern void program();

namespace nice {
    typedef long app_id;

    class app {
    public:
        static std::vector<std::string> args;
        static int ret_code;
    };

    int app::ret_code = 0; // Default return code.
    std::vector<std::string> app::args;
}
~~~

Now let's add the real `main()` function to our header file, and inside it, 
populate the app structure before passing control to our `program()` function.

~~~cpp
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
~~~

There's a lot going on here. First we separated the Windows and Linux
implementation by using the preprocessor. We copied command line arguments
to a vector args. We forwarded control to the `program()` function. And, finally,
we used return code from the app class.

## Evolving the app class

In the code fragment above four parameters are passed to the WinMain function.
And all four are ignored. Actually we implicitly use the lpCmdLine parameter, 
which points to the raw command line arguments, with a little help of our friends 
__argv and __argc, which point to the same content, but already processed for us. 
It saves us some effort.

One of cool features of Windows was providing os with an application instance and 
previous instance. Converted to a parlance of the common folks, this was a simple 
way to obtain unique process identifier, and check if application is already running. 
But, unfortunately, that only worked in 16 bit version of Windows. Modern Windows
always sets hPrevInstance to NULL, and hInstance to an internal memory address of the
application which is not the same as process identifier. But never mind! They placed
this ideas into our heads and now we mush have these features.

On unix you obtain process id by calling the getpid(), and check if application is already 
running by creating a temp file and locking it. if file is locked, application is 
already running. And if the application crashes the file will be automatically 
unlocked.

On Windows you obtain process id with a system call GetCurrentProcessId(), and
check . The hInstance of the exe can be obtained at any time by calling the
GetModuleHandle(NULL) so we don't need to store it.

To add these two features to the app class let us create an abstraction for the 
application id.

~~~cpp
#if _WIN32
    typedef DWORD  app_id;
#elif __unix__ 
    typedef pid_t app_id;
#endif
~~~

Now we can extedn our app class. We will add a function that 

~~~cpp
class app {
public:
    static std::vector<std::string> args;
    static int ret_code;
    static app_id id() { return id_; }
    static bool is_already_running();

private:
    static app_id id_;
};

int app::ret_code = 0;
std::vector<std::string> app::args;
~~~

### Unique application id

We use the process id as unique application id. Following function impelements
querying the operating system for application id.

~~~cpp
app_id app::id() {
#if _WIN32
    return ::GetCurrentProcessId();
#elif __unix__
    return ::getpid();
#endif
}
~~~

### Is application already running?

This check is done with a named mutex object on Windows, and a pseudo pid file
on Unix. For both platforms, the name is derived by stripping the first command line
argument (the executable filename!) of its extension. Hence the application name for 
`lesson1.exe` is `lesson1`, the named mutex name is `Local\lesson1`, and the 
pid file is `/tmp/lesson1.pid`.

~~~cpp
bool app::is_already_running() {
    std::string aname = std::filesystem::path(args[0]).stem().string();
#if _WIN32
    // Create local mutex.
    std::ostringstream name;
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
~~~

The reservation of resources - mutex creation and pid file creation and locking - is made 
inside the `is_already_running()` function, hence the first application to call this
function is officially the first running applicaiton of its kind. Also, for simplicity 
sake, we don't allow multiple calls to this function i.e. they all return false after
the first call.

## Conclusion

This is it for the first lesson of the nice library. In this lesson you learned how nice
is initialized, what the application class offers to you and how platform specific code is
isolated.

Next week we will write code for the base window class, and introduce a nice event handling 
mechanism.

## Listing

~~~cpp
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
        std::string aname = std::filesystem::path(args[0]).stem().string();
#if _WIN32
        // Create local mutex.
        std::ostringstream name;
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
~~~