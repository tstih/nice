# The Application

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
    class app {
    public:
        static std::vector<std::string> args;
        static int ret_code;
    };

    int app::ret_code = 0; // Default return code.
    std::vector<std::string> app::args;
}
~~~

Now let's add the real `main()` function to our framework. Inside it we'll 
first populate the app structure, and then pass control to our `program()` 
function.

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

What's going on here? First we separate Windows and Linux specific implementation 
details by using the preprocessor. We copy command line arguments to a vector named
`args`. And then we forward control to our `program()` function. Finally,
we pick the `ret_code` from the app class, and use it as the application 
return value.

 > To change the return value, assign the new value to the `ni::app::ret_code` in your `program()` function.

## Evolving the app class

In the code fragment above four parameters are passed to the `WinMain()` function.
And all four are ignored. Actually we implicitly use the `lpCmdLine` parameter, 
which points to the raw command line arguments. But we do it with a little help 
of our friends the `__argv` and `__argc`, which point to classic C command line 
arguments.

One of the coolest features of Windows was providing our start up function with 
the application handle of currently running application, and previous application handle
if one has already been running. This is what `hInstance`, and `hPrevInstance` were all about.
They were giving us a unique application identifier and ability to check if an instance
of this application is already running, on a plate.

Unfortunately, it only worked in 16 bit version of Windows. Modern Windows
always sets `hPrevInstance` to NULL, and `hInstance` to an internal memory address of the
application which is not the same as process identifier. But they did manage to place
the need for this into our brains and now our library must have these two features.

To add them to the app class let us create an abstraction for the application id, and 
add new members to the app class.

~~~cpp
#if _WIN32
    typedef DWORD  app_id;
#elif __unix__ 
    typedef pid_t app_id;
#endif

class app {
public:
    static std::vector<std::string> args;
    static int ret_code;
    static app_id id() { return id_; } // Return unique process id.
    static bool is_primary_instance();  // Is application already running?

private:
    static bool pinst_; // Are we the primary instance?
    static app_id id_;
};

bool app::pinst_ = false;
int app::ret_code = 0;
std::vector<std::string> app::args;
~~~

### Unique application id

On unix we can obtain process id by calling the `getpid()`, and on Windows we have
the `GetCurrentProcessId()` function for that.

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

The check if application is already running is quite different on Windows and on
Unix. On the latter we create a file (if it doesn't exist) and try to lock it.
If the lock is successfuly then we are the first instance. If lock fails another
instance is running.

On Unix the file is traditionally stored to /var/run and has an extension pid for
system services, and to /tmp folder for applications. It's content is by convention
a string value of process id that locked it. if the application crashes the file is 
automatically unlocked by the operating system.

On Windows we do this check by trying to obtain a named mutex object. If successful
then we are the first instance. If not, someone already obtained the mutex.

For both platforms, the application name is required. It is derived by stripping the 
first command line argument (the executable filename!) of its extension. Hence the 
application name for `lesson1.exe` is `lesson1`, the named mutex is `Local\lesson1`, 
and the pid file is `/tmp/lesson1.pid`.

Last but not least, if we are already the primary instance then we don't need to
try to obtain the lock anymore if called again. But if we are not, we try every
time.

Here's the code implementing this logic.

~~~cpp
bool app::is_primary_instance() {
    // Are we already primary instance? If not, try to become one.
    if (!pinst_) {
        std::string aname = std::filesystem::path(args[0]).stem().string();
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
~~~

The reservation of resources - obtaining the mutex and locking the pid file - is made 
inside the `is_primary_instance()` function, hence the first application to call this
function is officially the primary instance of its kind. For simplicity 
sake, we add initial call to this function into the `WinMain()` / `main()`.

## Conclusion

In this first lesson you learned how nice is initialized, what the application class offers 
to you and how platform specific code is isolated.

Next week we will write about windows and event handling.

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
        static bool is_primary_instance();

    private:
        static app_id id_;
        static bool pinst_;
    };

    int app::ret_code = 0;
    bool app::pinst_ = false;
    std::vector<std::string> app::args;

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
            std::string aname = std::filesystem::path(args[0]).stem().string();
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
~~~