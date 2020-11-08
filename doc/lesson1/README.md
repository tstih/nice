# The Desktop Application

When the operating system yields control to your application, 
it executes some bootstrap code and then jumps to your applications' 
entry point. In a traditional C/C++ application, this entry point 
is a function called `main()`.

Main accepts command line arguments, and returns an integer. 
This integer is called the return code and is passed back
to the operating system. A zero return value signals success 
and any other value could indicate an error.

Some operating systems don't follow this convention. For example, 
MS Windows main entry point of a desktop application is a function
called `WinMain()`. 

Having different entry points on each platform would defeat our 
purpose of being nice. So the first task of our framework is to
abstract the main function.

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

Now we add the real `main()` function to our header file. Inside it, we 
populate the app structure, and pass control to our `program()` function.

~~~cpp
#if _WIN32 || WIN32
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    nice::app::args=std::vector<std::string>(__argv + 1, __argv + __argc);
#elif __unix__
int main(int argc, char *argv[]) {
    nice::app::args=std::vector<std::string>(argv + 1, argv + argc);
#else
#error "Only Windows and Linux supported."
#endif
    // Run program.
    program();

    // And return return code;
    return nice::app::ret_code;
}
~~~

We adjusted our code to the environment by using preprocessor symbols
_WIN32, WIN32, and __unix__. 

## Let's be a bit nicer

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
#ifdef _WIN32
typedef DWORD  app_id; 
#elif linux 
typedef pid_t app_id;
#else
#error "Only Windows and Linux supported."
#endif
~~~

Now we can add it to our app class, and populate it inside our real main function code.

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

## Time to be nice

# Copy'n' paste links

Special thanks to anonymous programmers who provided code for the following.

1. Checking if process already runs on Linux.
https://arstechnica.com/civis/viewtopic.php?t=42700

2. 

## Listing

~~~cpp
#include <string>
#include <vector>

extern void program();

namespace nice {
    typedef long app_id;

    class app {
    public:
        // Cmd line arguments.
        static std::vector<std::string> args;

        // Return code.
        static int ret_code;

        // Process id.
        static app_id id() { return id_; }

        // Is another instance already running?
        static bool is_already_running();

    private:
        static app_id id_;
    };

    int app::ret_code = 0;
    std::vector<std::string> app::args;
}

#if _WIN32 || WIN32
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    nice::app::args=std::vector<std::string>(__argv + 1, __argv + __argc);
#elif __unix__
int main(int argc, char *argv[]) {
    nice::app::args=std::vector<std::string>(argv + 1, argv + argc);
#endif
    // Run program.
    program();

    // And return return code;
    return nice::app::ret_code;
}

void program() {
    // Our code.
}
~~~