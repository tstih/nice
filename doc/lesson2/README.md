# The Window

In Lesson One we created our application class and wrote some 
startup code. Now let us get some instant gratification by
displaying our very first window.

Our starting point will be the code that we wrote in Lesson
One. A desktop application usually creates a main window and then
spends its life in an event harvesting loop, reacting 
to user actions. To accomodate this we are going to add
a window class, pass it to our application class, and 
create an event harvesting loop.

Since Unix does not have a native GUI, we first need to 
extend our `CMakeList.txt` to link one. We will use 
the [GTK3 GUI library](https://www.gtk.org/).

## Extending CMakeList.txt

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
    FIND_PACKAGE(PkgConfig REQUIRED)
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

To create a C++ object representing a system resource, we first
need to create a plain C++ object, and then create and attach 
the underlying system resource. 

Due to the way how C++ works this can't always be done inside 
a constructor. For example, as soon as we call the `::CreateWindow()` 
function in MS Windows, it sends a message to the window. If we 
were to call this function in a constructor, a message would be sent 
to a C++ object that has not been completely constructed.

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

Our window class simply has to derive from this class and 
implement the create. Type T is the type of the underlying 
system resource. For example, we could implement a window
like this:

~~~cpp
class wnd : resource<HWND> {
public:
    HWND create() override {}; // Create window in this func.
    void destroy() override noexcept {}; // Destroy wnd in this func.
}
~~~

But it would be not be very nice to expect of a programmer to
explicitly call `create()` and `destroy()` functions
every time. And that goes against our philosopy of, well, 
being nice. We want the creation process to be invisible to 
the programmer. And one way to achieve this is by using lazy
evaluation. The sole purpose of resource is to expose a system
resource. Let's call it the id of the resoruce, because, well
it usually is a handle or a pointer to a resource. So we could 
simply hold this system resource of type T inside our resource
class, and call `create()` when it is accessed for the first time.

And, because our class derives from the `resource` class, we
could call `destroy()` in a destructor. 

~~~cpp
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
~~~

Our window class would then look like this.

~~~cpp
#if _WIN32
    typedef HWND wnd_id;
#elif __unix__ 
    typedef GtkWidget* wnd_id;
#endif

class wnd : public resource<wnd_id> {
public:
    virtual wnd_id create() = 0; // Concrete window will implement this.
    void destroy() noexcept override {
#if _WIN32
        ::DestroyWindow(id(true)); id(nullptr);
#endif
        }
    };
};
~~~

Creating a window when you access window system resource for the
first time makes alot of sense. All Win32 functions accept `HWND`
so you can't call any native method on a window without referencing
the `HWND`. On the other side, if you never reference `HWND` the 
expensive underlying system object is never created.

 > Destruction pattern is a bit less perfect. It goes again good
 > practice of C++ programming that says you should never call
 > virtual functions from ctors and dtors, as wrong function may
 > get called. It is a compromise that works, because all system
 > resources of certain type (for example: all windows or all
 > device contexts) are destroyed using the same function.

## Base window class





