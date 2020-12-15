# The Window

In first lesson we created an application class and wrote 
startup code. In second lesson we will create and display 
our first window. 

Our starting point will be the code that we wrote in previous
lesson. A desktop application usually creates a main window 
and then spends its life in an event harvesting loop, 
reacting to user actions. To accomodate this we are going 
to add a window class, pass it to our application class, 
and create an event harvesting loop.

The objective of this lesson is to make this simple code
compile and display main window.

~~~cpp
#include "nice.hpp"

using namespace ni;

void program() {
    app::run(app_wnd("Hello World!",{640,400}));
}
~~~

## Unix support

Since Unix does not have a native GUI, we first need to 
extend our `CMakeList.txt` to link one. We will use 
the [GTK3 GUI library](https://www.gtk.org/).

### Extending CMakeList.txt

On Unix we must first install the GTK3 development files: 

`sudo apt-get install libgtk-3-dev`

Then we can change our `CMakeList.txt` file.

~~~CMake
cmake_minimum_required (VERSION 3.8)
project(lesson2)
if (WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_executable (lesson2 WIN32 lesson2.cpp)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    add_executable (lesson2 lesson2.cpp)
    target_link_libraries(lesson2 ${GTK3_LIBRARIES})
endif()
set_property(TARGET lesson2 PROPERTY CXX_STANDARD 20)
~~~

We use the `PkgConfig` tool to find all necessary
settings for the `gtk+-3.0` library. If you'd like to
know more about the `pkg-config` tool, [click here]
(https://people.freedesktop.org/~dbn/pkg-config-guide.html).

On Windows we use the native GUI and there are no changes required.

## Two phase construction pattern

Our window C++ class will have to wrap underlying window system resource.
To create it, we will first create a (plain!) C++ object, and then 
create and attach the underlying window system resource. 

Due to the way how C++ works we can't create the system 
resource inside the C++ class constructor. One reason is because 
as soon as we call the `::CreateWindow()` function in MS Windows, 
it sends a create message to the window. If we were to call this 
function in a constructor, a message would be sent to a C++ object 
that has not yet been completely constructed.

Hence our class need to be created in two steps. First we create
C++ object instance. And then we create and attach a windows
system resource.

For this purpose we require every C++ class that wraps system
resource to implement a `create()` function and a `destroy()`
function.

~~~cpp
template<typename T>
class resource {
public:
    // Create and destroy pattern.
    virtual T create() = 0;
    virtual void destroy() noexcept = 0;
}
~~~

The T type is type of system resource (for example `HWND` or
`GtkWidget*`),

Our window class derives from this class and implements the create.

~~~cpp
#if _WIN32
    typedef HWND wnd_instance;
#elif __unix__ 
    typedef GtkWidget* wnd_instance;
#endif

class wnd : public resource<wnd_instance> {
public:
    // Implement create and destroy functions.
}
~~~

This pattern takes care of separation of the two constructors:
 * a C++ class constructor and 
 * a `create()` function. 
It also forces every derived class to implement the `create()` and 
`destroy()` functions. Which is exactly what we want.

But how do we actually construct the class after we have implemented
two phase construction interface? Expecting from programmer to remember 
to initialize every class twice, and manually call `destroy()` 
at the end of its lifetime it is not very nice.

We want the creation process to be invisible to the programmer. 
But how can we do this if we are not allowed to call `create()`
in the C++ constructor? With lazy evaluation of key class property.

Every class that wraps system resource has a pointer to this
resource. And this pointer is used in all system API calls to
handle this resource. So it is quite crucial for this resource.
For a window system resource in MS Windows, this resource is
`HWND`. For the GTK libray in unix this resource is `GtkWindow*`.

It is a fair assumption that we can create window first time this
key class property is read.

Now let's create a lazy evaluating resource class.

~~~cpp
template<typename T>
class resource {
public:
    // Create and destroy pattern.
    virtual T create() = 0;
    virtual void destroy() noexcept = 0;
    // Instance setter.
    virtual void instance(T instance) const { instance_ = instance; }
    // Instance getter with lazy eval.
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
    // Store resource key property here.
    mutable T instance_{ nullptr };
};
~~~

Creating a window system resource when you access it for the
first time makes alot of sense. If you never read it - the 
expensive underlying system resource is never created.

 > Destruction pattern is a bit less perfect. It goes again good
 > practice of C++ programming that says you should never call
 > virtual functions from ctors and dtors, as wrong function may
 > get called. It is a compromise that works, because all system
 > resources of certain type (for example: all windows or all
 > device contexts) are destroyed using the same function.

## Base window class

Now that we have the two phase construction pattern, we can create
base window class.

~~~cpp
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

// Create function.
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

    // Set title and height.
    ::gtk_window_set_title (GTK_WINDOW (w), title_.c_str());
    ::gtk_window_set_default_size (GTK_WINDOW (w), size_.width, size_.height);

    // Make it closeable.
    ::g_signal_connect(w, "destroy",G_CALLBACK(::gtk_main_quit), NULL);  
    return w;
#endif
}
~~~

In the code above we derive base window class `wnd` from the `resource` class
and implement virtual `create()` and concrete `destroy()` function. We then
derive `app_wnd` window from `wnd` class and implement concrete `create()`
window which creates top window resource and connects quit messages so that
the application can be closed. Other window messages are forwarded to default
handlers.

