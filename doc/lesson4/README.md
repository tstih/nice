# Porting nice!

Hello, again! If you are reading this then you've read your way through lessons
one, two, and three. It's now time to repeat everything we've learned, and there's
no better way of doing this than to port nice to a new platform. Thus in this lesson
we are going to port nice to X11.


# Enriching CMakeLists.txt

Before making changes to the code, we need to update our CMakeLists.txt to include 
X11 bindings. Let's keep GTK as default Unix port, and introduce the option to 
override it. Here is the updated CMakeLists.txt.

~~~
# We require 3.8.
cmake_minimum_required (VERSION 3.8)

# Project name is lesson 4.
project(lesson4)

# Were command line args passed?

if (WIN32 OR WIN)
    add_definitions(-DWIN32_LEAN_AND_MEAN -D__WIN__)
    add_executable (lesson4 WIN32 lesson4.cpp nice.hpp)

elseif(X11)
    add_definitions(-D__X11__)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    link_directories(${X11_LIBRARIES})
    add_definitions(${GTK3_CFLAGS_OTHER})
    add_executable (lesson4 lesson4.cpp nice.hpp)   
    target_link_libraries(lesson4 ${X11_LIBRARIES})

    # Debug version.
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_BUILD_TYPE Debug)

elseif(GTK OR UNIX)
    add_definitions(-D__GTK__)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    add_executable (lesson4 lesson4.cpp nice.hpp)
    target_link_libraries(lesson4 ${GTK3_LIBRARIES})

    # Debug version.
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_BUILD_TYPE Debug)

endif()

set_property(TARGET lesson4 PROPERTY CXX_STANDARD 20)
~~~

Because we now have three options, the `cmake` can't automatically decide what
to build. If it detects MS Windows it still builds a version for MS Windows, and
if it detects Unix it builds the GTK3 version. But there is no automation
for X11. So we have to pass this as an argument to cmake, like this `cmake -DX11:1`.

 > Similarly we can implicitly require to build versions for GTK (`cmake -DGTK:1`) 
 > and MS Windows (`cmake -DWIN:1`). 


# How to start?

A practical way to create a port is to examine all places with platform
dependant code. You can find them by searching for `__WIN__` or `__GTK__`
processor directives.

## Includes

We need core X11 files. This is the new include section.

~~~cpp
#if __WIN__
#include <windows.h>
#elif __GTK__
#include <sys/file.h>
#include <unistd.h>
#include <gtk/gtk.h>
extern int main(int argc, char* argv[]);
#elif __X11__
#include <sys/file.h>
#include <unistd.h>
#include <X11/Xlib.h>
#endif
}
~~~

## Unified types

As with previous two ports, most unified types are mappings to native types. 
But ... there is an exception. The`canvas` is a pointer to a custom structure, 
because X11 drawing procedures require extra data, and this is the best way 
to pass it to the artist. 

~~~cpp
#if __WIN__
    typedef DWORD  app_id;
    typedef HINSTANCE app_instance;
    typedef HWND wnd_instance;
    typedef LONG coord;
    typedef BYTE byte;
    typedef HDC canvas;
#elif __GTK__ 
    typedef pid_t app_id;
    typedef GtkApplication* app_instance; // Not used.
    typedef GtkWidget* wnd_instance;
    typedef int coord;
    typedef uint8_t byte;
    typedef cairo_t* canvas;
#elif __X11__
    typedef pid_t app_id;
    typedef Display* app_instance;
    typedef Window wnd_instance;
    typedef int coord;
    typedef uint8_t byte;
    typedef struct x11_canvas {
        Display* d;
        Window w;
        GC gc;
    } canvas;
#endif
~~~

## Extending the resource class

Surely, you remember the `resource` type? The one we use for the
two phase construction? This class worked relly well for the first 
two ports. But hard-coding `nullptr` for default value into it was 
a bit optimistic. Because it excluded  value types, such as XWindows  
`Window` structure.

Let's fix this by introducing another template parameter, 
which tells the class which is the default value for the `instance_`
member. 

~~~cpp
template<typename T, T N=nullptr>
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
        if (instance_ == N)
            instance_ = const_cast<resource<T,N>*>(this)->create();
        // Return.
        return instance_;
    }

    bool initialized() {
        return !(instance_ == N);
    }

private:
    // Store resource value here.
    mutable T instance_{ N };
};
~~~

To maintain compatibility, let's set the default value of new parameter 
to `nullptr`. 


## Application

There are two places where we need to look when creating a new port:
 * Application entry point (i.e. the `main()` function) where we 
   need to take care of platform specific initialization,
 * The application `run()` function where we need to update 
   the main message loop, and

 > Optionally, some application functions might need to change 
 > (for example: `is_primary_instance()`), but not for X11 port.

### Application entry point (and setting an instance!)

The underlying type for application instance for X11 is `XDisplay*` 
and we need to set it inside the `main()` function. After the event loop 
finishes, we also need to close the display properly. 

~~~cpp
int main(int argc, char* argv[]) {
    // X Windows initialization code.
    ni::app::instance_=XOpenDisplay(NULL);

    // Copy cmd line arguments to vector.
    ni::app::args = std::vector<std::string>(argv, argv + argc);

    // Try becoming primary instance...
    ni::app::is_primary_instance();
    
    // Run program.
    program();
    
    // Close display.
    XCloseDisplay(ni::app::instance());

    // And return return code;
    return ni::app::ret_code;
}
~~~

 > For `main()` to be able to set the `app::instance_` it has to be friend 
 > of the `app` class.


### The message pump

The message pump is implemented inside the `app::run()` method.

~~~cpp
void app::run(const app_wnd& w) {
    Atom atom = XInternAtom ( app::instance(),"WM_DELETE_WINDOW", false );
    XSetWMProtocols(app::instance(), w.instance(), &atom, 1);

    // We're interested in...
    XSelectInput ( app::instance(),
            w.instance(),
        ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | 
        LeaveWindowMask | PointerMotionMask | FocusChangeMask | KeyPressMask |
        KeyReleaseMask | SubstructureNotifyMask | StructureNotifyMask | 
        SubstructureRedirectMask);

    // Show window, and force flush.
    ::XMapWindow(app::instance(), w.instance());
    ::XFlush(app::instance());

    XEvent e;
    bool quit=false;
    while ( !quit ) // Will be interrupted by the OS.
    {
        ::XNextEvent ( app::instance(),&e );
        quit = global_handle_x11_event(e);
    }
}
~~~

The message pump initially registers the `WM_DELETE_WINDOW` atom, which is  
*a very dirty* way to detect XWindows closing the main window. 
After we've registered it, we select which events we want to listen to by 
calling the `XSelectInput()`.
We than show the main window by calling the `XMapWindow()`, and enter the
message loop. Inside, we pass each event to the `global_handle_x11_events()` 
handler. As we will see later, this handler routes events to local 
window procedures, just like handlers for Windows and GTK.

 > The current `XSelectInput()` call is a bit of an overkill. We subscribe to 
 > more events than needed. Let's call this an investment into the not so 
 > distant future. 

### Primary instance check

Primary instance checks for X11 and GTK are the same. So the only change
is changing preprocessor check from `#if __GTK__` to `#if __GTK__ || __X11__`.

## Window

The XWindow `Window` doesn't have a nice way of storing data with the native
structure. So we must associate the native `Window`, and our `wnd` class
using a global map.

 > Actually there is a dirty way of storing data into the `Window` structure,
 > but we choose to avoid it. We could store an additional pointer to the window
 > title, after the 0 character, and no one would know it is there.

~~~cpp
std::map<wnd_instance,wnd*> wmap;
~~~

A new mapping is added to this map every time the `wnd::create()` is called.
Likewise when the `wnd::destroy()` is called, the mapping for that window 
is removed from the map.

### Events

In the *Message pump* chapter we created the `app::run()`, which calls our 
 `global_hnadle_x11_event()` and passes events to it. We can now utilize our 
 global map `wmap` to route these events to their respective local handlers.  

~~~cpp
bool global_handle_x11_event(const XEvent& e) {
    Window xw = e.xany.window; // Get native structure from the event.
    wnd* w=wmap[xw]; // Find attached class.
    return w->local_handle_x11_event(e); // And pass it the event.
}
~~~

The local handler `wnd::local_handle_x11_event` maps (standard!) events to our
signals. Currently, these events are `created`, `destroyed` and `paint`. 

~~~cpp
bool wnd::local_handle_x11_event(const XEvent& e) {
    bool quit=false;
    switch ( e.type )
    {
    case CreateNotify:
        created.emit();
        break;
    case ClientMessage:
        {
            Atom atom = XInternAtom ( app::instance(), "WM_DELETE_WINDOW", false );
            if ( atom == e.xclient.data.l[0] )
                destroyed.emit();
                quit=true;
        }
        break;
    case Expose:
        {
            canvas c { app::instance(), instance(), 
                XCreateGC(app::instance(), instance(), 0, NULL) }; 
            artist a(c);
            paint.emit(a);
            XFreeGC(app::instance(),c.gc);
        }
        break;
    }
    return quit;
}
~~~

### Create window

Following is a school case *(no, really, I copied it from a student's page!)* 
X11 code for creating a native window. Note that we automatically insert every 
created window into the global map.

~~~cpp
wnd_instance app_wnd::create() {
    int s = DefaultScreen(app::instance());
    Window w = ::XCreateSimpleWindow(
        app::instance(), 
        RootWindow(app::instance(), s), 
        10, // x 
        10, // y
        size_.width, 
        size_.height, 
        1, // border width
        BlackPixel(app::instance(), s), // border color
        WhitePixel(app::instance(), s)  // background color
    );
    // Store window to window list.
    wmap.insert(std::pair<Window,wnd*>(w,this));
    return w;
}
~~~

### Destroy window

When the window is destroyed it also removes itself from the map to
stop routing events to a non-existing object instance.

~~~cpp
void wnd::destroy() noexcept {
    if (initialized()) {
        // Remove from windows map.
        wmap.erase (instance()); 
        // And destroy.
        XDestroyWindow(app::instance(), instance()); instance(0);
    } 
}
~~~

## Artist

Finally, the artist class is extended to handle painting using XWindows
poor set of drawing functions. Besides modifying draw rectangle function
to call X11 natives, we also need to create a *private* function to help 
us translate our `color` to `XColor`.

The code for both is below.

~~~cpp
// Draw a rectangle.
void artist::draw_rect(color c, rct r) const {
    XColor pix=to_xcolor(c);
    XSetForeground(canvas_.d, canvas_.gc, pix.pixel);
    XDrawRectangle(canvas_.d, canvas_.w, canvas_.gc, r.x, r.y, r.w, r.h); 
    XFlush(canvas_.d); // Maybe not needed?
}
// ... code omitted ...
XColor artist::to_xcolor(color c) const {
    XColor clr;
    std::ostringstream sclr;
    sclr << '#' << std::hex << std::setfill('0') << std::setw(2) << (int)c.r 
        << std::setfill('0') << std::setw(2) << (int)c.g
        << std::setfill('0') << std::setw(2) << (int)c.b;
    auto colormap = XDefaultColormap(app::instance(), DefaultScreen(app::instance()));
    const char* clr_name=sclr.str().c_str();
    XParseColor(app::instance(), colormap, clr_name, &clr);
    XAllocColor(app::instance(), colormap, &clr);
    return clr;
}
~~~

 > We can live with XWindows drawing functions for now. But in the future
 > we will use Pango/Cairo libraries as the aging XWindows lacks power
 > to support requirements of a modern user interface. 


# Done!

That's it. We've ported nice to X11. Now we can execute following code on it.

 ~~~cpp
 #include "nice.hpp"

using namespace ni;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello world!", { 800,600 })
    {
        // Subscribe to the paint event.
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    // Paint handler, draws red, green and blue rectangles.
    bool on_paint(const artist& a) {
        a.fill_rect({ 255,255,255 },{ 0, 0, 800, 600});
        a.draw_rect({ 255,0,0 }, { 100,100,600,400 });
        a.draw_rect({ 0,255,0 }, { 150,150,500,300 });
        a.draw_rect({ 0,0,255 }, { 200,200,400,200 });
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}
 ~~~ 

Thank you for your patience.

In our next lesson (Lesson 5) we will add some child controls, and create
a cool layout manager. 

Stay tuned.