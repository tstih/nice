# Porting nice!

Hello again. If you are reading this then you've read your way through lessons
one, two, and three. It's now time to repeat everything we learned, and there's
no better way to do this than to port nice to a new platform. So in this lesson
we are going to port it to X11 together.

# Enriching CMakeLists.txt

Let's enrich our CMakeLists.txt to include X11 port and make this port default
unix port.

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
if it detects Unix it tries to build the GTK3 version. But there is no automation
for X11. You have to pass this request as argument to cmake `cmake -DX11:1`.

 > Sinilarly you can implicitly require to build versions for GTK (`cmake -DGTK:1`) 
 > and MS Windows (`cmake -DWIN:1`). 

# How to start?

Starting the port is failry easy. You need to examine all the places with platform
dependant code. You can find those places by searching for `__WIN__` or `__GTK__`
processor directives.

## Includes

You'll only need to include core X11 files at this point.

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

Most unified types are simple mappings to native type. An exception is type
`canvas`, which is a structure, holding multiple data, needed for the artist
to paint. It is neatly packed so that the underlying logic is invisible to the
user, who simply operates on canvas.

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

Because our resource type had hard-coded `nullptr` for default
value, it did not work with value type. But the `Window` structure of
XWindows is not a pointer but a value type, incompatible with `nullptr`.

That is why we introduce another template parameter, which tells what is
resources' `nullptr` value. Like this.

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

To maintain downward compatibility, we set the default value of new
parameter to `nullptr`.

## Application

Application entry point needs to take care of some XWindows initialization 
activities, and the `run` function needs to implement XWindows main
event loop.

### Application instance and entry point

The default type for application instance is `XDisplay*` and we set
it inside the `main()` function. After the event loop finishes, we
also close the display. Here is the code.

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

 > For `main()` to be able to set the `app::instance_` private member
 > it has to be friend of the `app` class.

### Primary instance check

Primary instance check for X11 and GTK is the same. So all that we need to do
is include `__X11__` in the preprocessor check.

~~~cpp
bool app::is_primary_instance() {
    // Are we already primary instance? If not, try to become one.
    if (!primary_) {
        std::string aname = app::name();
#if __WIN__
        // Create local mutex.
        std::ostringstream name;
        name << "Local\\" << aname;
        ::CreateMutex(0, FALSE, name.str().c_str());
        // We are primary instance.
        primary_ = !(::GetLastError() == ERROR_ALREADY_EXISTS);
#elif __GTK__ || __X11__ // The only change!
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
~~~

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

The main body the pump registers `WM_DELETE_WINDOW` atom, which is a know 
(weird!) way to catch XWindows closing the main window. We then select which
events we want to receive by `XSelectInput()`.

After that we show window with `XMapWindow()`, and enter event loop. We pass
events to the `global_handle_x11_events()` handler, and it forwards them
to correct window class event handler.

 > The current message pump for X11 is a bit of an overkill. We subscribe to 
 > more events than needed because we will handle them in the future. 

## Window

The XWindow `Window` doesn't have a nice way of storing data with the native
structure. So we will connect native structure `Window`, and our `wnd` class
using a global map.

~~~cpp
std::map<wnd_instance,wnd*> wmap;
~~~

This map is global and a mapping is added to it whenever `wnd::create()` is called.
Likewise when the `wnd::destroy()` is called, the mapping for that window is removed 
from the map.

### Events

We are calling our `global_hnadle_x11_event()` in the `app::run()`, and passing
it every event. We can now utilize our global map `wmap` to route events to their 
respective local handlers.  

~~~cpp
bool global_handle_x11_event(const XEvent& e) {
    Window xw= e.xany.window;
    wnd* w=wmap[xw];
    return w->local_handle_x11_event(e);
}
~~~

The base local handler `wnd::local_handle_x11_event` maps standard events to our
signals. Observe the `Expose` event (the paint event). It creates a new canvas,
passes it to the artist, and frees it. 

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

### Create

An application window is created by calling the `::CreateSimpleWindow',
and inserting it into the map. 

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

### Destroy

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

And, finally, the artist class is extended to handle painting using XWindows
poor set of functions. We create a *private* function to help us translate
our `color` structure to `XColor`, and modify draw rectangle function for X11.

~~~cpp
// Draw a rectangle.
void artist::draw_rect(color c, rct r) const {
#if __WIN__
    RECT rect{ r.left, r.top, r.x2(), r.y2() };
    HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
    ::FrameRect(canvas_, &rect, brush);
    ::DeleteObject(brush);
#elif __GTK__
    cairo_set_source_rgb(canvas_, normalize(c.r), normalize(c.g), normalize(c.b));
    cairo_set_line_width(canvas_, 1);
    cairo_rectangle(canvas_, r.left + 0.5f, r.top + 0.5F, r.width, r.height );
    cairo_stroke(canvas_);
#elif __X11__
    XColor pix=to_xcolor(c);
    XSetForeground(canvas_.d, canvas_.gc, pix.pixel);
    XDrawRectangle(canvas_.d, canvas_.w, canvas_.gc, r.x, r.y, r.w, r.h); 
    XFlush(canvas_.d); // Maybe not needed?
#endif
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

 > For demo purposes using XWindows drawing functions is fine, but in the future
 > we will use Pango/Cairo for painting. The aging XWindows simply lacks power
 > to support requirements of modern user interfaces. 

# Test!

 And we are done. The X11 port test works. We can now execute this code on
 three systems: MS Windows, GTK (any system), and raw X11.

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

 In next lesson (Lesson 5) we will add child controls, and crate
 a layout manager for our window.