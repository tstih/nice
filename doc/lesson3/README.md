# Let's Paint!

Hello, again! In previous two lessons we have learned how 
to create a platform independent application class and a 
window base class. In this lesson we are going to create
a platform independent event handling mechanism for
windows and paint some lines to its surface.

Our starting point will be -as usual- the code that we wrote 
in previous lesson. Our test case (code that must compile and
work after this lesson) will be:

 > In this lesson we may often refer to an event as a message. Same thing.

~~~cpp
#include "nice.hpp"

using namespace ni;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello world!", {800,600})
    {
        // Subscribe to the paint event.
        paint.connect(this, &main_wnd::on_paint);
    }
private:    
    // Paint handler, draws red, green and blue rectangles.
    void on_paint(const artist& a) {
        a.draw_rect({ 255,0,0 }, { 100,100,600,400 });
        a.draw_rect({ 0,255,0 }, { 150,150,500,300 });
        a.draw_rect({ 0,0,255 }, { 200,200,400,200 });
    }
};

void program()
{
    app::run(main_wnd());
}
~~~

## Signals and slots

It was W. H. Davenport Adams who first said: "Good artists copy; 
great artists steal," not Steve Jobs, and not Pablo Picasso. 
So let us be good artists, and reuse an existing [signals
and slots implementation](https://schneegans.github.io/tutorials/2015/09/20/signal-slot)  
by Simon Schneegans. Please read his blog first. 

His plain implementation is compliant with the nice philosophy: it is short,
and utilizes a feature of modern C++: variadic templates.

Here is the stripped down version of his code.

~~~cpp
template <typename... Args>
class signal {
public:
    signal() : current_id_(0) {}
    signal(std::function<void()> init) : signal() { init_ = init; }
    template <typename T> int connect(T* inst, bool (T::* func)(Args...)) {
        return connect([=](Args... args) {
            return (inst->*func)(args...);
            });
    }

    template <typename T> int connect(T* inst, bool (T::* func)(Args...) const) {
        return connect([=](Args... args) {
            return (inst->*func)(args...);
            });
    }

    int connect(std::function<bool(Args...)> const& slot) const {
        if (!initialized_ && init_ != nullptr) { init_(); initialized_ = true; }
        slots_.insert(std::make_pair(++current_id_, slot));
        return current_id_;
    }

    void disconnect(int id) const {
        slots_.erase(id);
    }

    void disconnect_all() const {
        slots_.clear();
    }

    void emit(Args... p) {
        // Iterate in reverse order to first emit to last connections.
        for (auto it = slots_.rbegin(); it != slots_.rend(); ++it) {
            if (it->second(std::forward<Args>(p)...)) break;
        }
    }
private:
    mutable std::map<int, std::function<bool(Args...)>> slots_;
    mutable int current_id_;
    mutable bool initialized_{ false };
    std::function<void()> init_{ nullptr };
};
~~~

We added two features to it 
 * The new emit function uses the first-in-last-out principle 
   when calling subscribers. We did this because derived windows
   typicaly subscribe after base windows, but should be the first
   to receive events. An event handler can return true if it 
   processed the message. In this case we break the call chain.
 * A lambda can be passed to this class so that we can catch first
   subscription and run some initial code. We will use this to
   subscribe to GTK signals.
 
We create a signal on the transmitter's side,
and listen (connect) to it on the receiver's side.  

We create a signal like this...

~~~cpp
class artist {
    // Implement artist's functions here.
};

class wnd : public resource<wnd_instance> {
public:
    // ... code omitted ...
    signal<const artist&> paint;
    // ... code omitted ...
}
~~~

...we raise it by calling `paint.emit`, and we listen to it 
like this...

~~~cpp
class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello world!", {800,600})
    {
        // Subscribe to the paint event.
        paint.connect(this, &main_wnd::on_paint);
    }
private:    
    // Paint handler, draws red, green and blue rectangles.
    void on_paint(const artist& a) {
        // Do your drawing.
    }
};
~~~

In this lesson we will simply catch native events on both platforms (Win32 and GTK),
and translate them to these signals. 

## Catching native events

### On MS Windows

#### The window procedure

Every window on MS Windows has a window procedure. This procedure is called 
by the main message loop (see prevous lesson) when a window receives a message.
On MS Windows we attach the window procedure to the window when we register
the window class.

We already did this in previous lesson! Silently. :) The code that creates
window class is inside our `app_wnd::create()` function. The member 
`wcex_.lpfnWndProc` points to window procedure called `global_wnd_proc()` 
that receives window messages. It currently just handles the `WM_DESTROY`
message as visible from the code fragment below.

~~~cpp
wnd_instance app_wnd::create() {
    // Get class.
    class_ = app::name();

    // Register window.
    ::ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
    wcex_.cbSize = sizeof(WNDCLASSEX);
    wcex_.lpfnWndProc = global_wnd_proc; // Here is the window procedure!
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
}

static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    }
    // If we are here, do the default stuff.
    return ::DefWindowProc(hWnd, message, wParam, lParam);
}
~~~

Every window needs a window procedure. Not only that - every window class (i.e. an 
application window, a button, a text box) needs a different window procedure. Hence 
it makes sense to move this procedure to the base `wnd` class instead of keeping it
in derived `app_wnd` class.

But wait! The window procedure must be specific to window class, and the `global_wnd_proc` 
is static. So there can be only one for all derived windows?

To mitigate this we must implement a mechanism similar to a railroad switch.  
`global_wnd_proc` will receive all messages for all `wnd` derived windows, and then  
simply forward each message to its' window class.

#### The instance specific window procedure

To implement instance specific window procedure our `global_wnd_proc` examines the very first 
message sent to any window! According to documentation, this is the `WM_NCCREATE`. 
This message brings with it a special structure of type `LPCREATESTRUCT`, and a member 
of this structure called `lpCreateParams` can be used to pass user specific data with this
message. We can set the value of this member when we call the `::CreateWindow` 
in our `create()` procedure. 

This is the code fragment call from the `app_wnd::create()`. Note that last parameter 
is `this`. This parameter is the one we pass together with the `WM_NCCREATE` message.
In plain words - when this message is received by the global window procedure that 
receives all messages for all windows, this parameter tells it to which window it
needs to forward the message.

~~~cpp
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
    this); // Here we pass value that will be passed inside lpCreateParams of WM_NCCREATE msg.
~~~

BUT... this parameter is only passed with the `WM_NCCREATE`. So how do we know to which
window we need to forward the message for all subsequent messages? Well, we need to store
pointer to window C++ class to MS Windows native window structure so that it's available
with every window message.

MS Windows allows us to store a value to internal window structure by using 
the `::SetWindowLongPtr()` API function. And we can also use `::GetWindowLongPtr()`
to read it. 

Here's the code to do it.

~~~cpp
class wnd : public resource<wnd_instance> {
public:
    virtual ~wnd() { destroy(); }
    virtual wnd_instance create() = 0;
    void destroy() noexcept override {
        if (initialized())
            ::DestroyWindow(instance()); instance(nullptr);
    }
protected:
    // Generic callback. Calls member callback.
    static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // Is it the very first message? Only on WM_NCCREATE.
        // TODO: Why does Windows 10 send WM_GETMINMAXINFO first?!
        wnd* self = nullptr;
        if (message == WM_NCCREATE) {
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            auto self = static_cast<wnd*>(lpcs->lpCreateParams);
            self->instance(hWnd); // save the window handle too!
            ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
            self = reinterpret_cast<wnd*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

        // Chain...
        if (self != nullptr)
            return (self->local_wnd_proc(message, wParam, lParam));
        else
            return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    // Local callback. Specific to wnd.
    virtual result local_wnd_proc(msg msg, par1 p1, par2 p2) {
        return ::DefWindowProc(instance(), msg, p1, p2);
    }
};
~~~

From now on every `wnd` derivate can override `local_wnd_proc()` and implement
class specific behaviour for a window. The `global_wnd_proc()` will handle the
message routing.

#### Implement signals, common to all windows

Some window messages are class specific, and some are sent to all windows.
Examples of messages sent to all windows are: 
 * message sent to window when it is created,
 * message sent to window when it is destroyed,
 * message sent to window when it needs to repaint itself,
 * message sent to window when the mouse is moved across the surface.
It makes sense to handle messages, that are common to all windows, 
inside the base `wnd` class. So (as a proof of concept!) let's do it 
just for `created` and `destroyed` to show how it's done.


~~~cpp
class wnd : public resource<wnd_instance> {
public:
    virtual ~wnd() { destroy(); }
    virtual wnd_instance create() = 0;
    void destroy() noexcept override {
        if (initialized())
            ::DestroyWindow(instance()); instance(nullptr);
    }
    // Signals.
    signal<> created;
    signal<> destroyed;
protected:
    // Generic callback. Calls member callback.
    static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // Is it the very first message? Only on WM_NCCREATE.
        // TODO: Why does Windows 10 send WM_GETMINMAXINFO first?!
        wnd* self = nullptr;
        if (message == WM_NCCREATE) {
            LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            auto self = static_cast<wnd*>(lpcs->lpCreateParams);
            self->instance(hWnd); // save the window handle too!
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
            self = reinterpret_cast<wnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        // Chain...
        if (self != nullptr)
            return (self->local_wnd_proc(message, wParam, lParam));
        else
            return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    // Local callback. Specific to wnd.
    virtual result local_wnd_proc(msg msg, par1 p1, par2 p2) {
        switch (msg) {
        case WM_CREATE:
            created.emit();
            break;
        case WM_DESTROY:
            destroyed.emit();
            break;
        default:
            return ::DefWindowProc(instance(), msg, p1, p2);
        }
        return 0;
    }
};
~~~

Simple, isn't it. We now have window code that rewires `WM_DESTROY` to 
the `destroyed` signal. So in the `app_wnd` we can close application 
when this signal is emitted.  

~~~cpp
class app_wnd : public wnd {
public:
    app_wnd(std::string title, size size) : wnd() {
        // Store parameters.
        title_ = title; size_ = size;
        // Subscribe to destroy signal.
        destroyed.connect(this, &app_wnd::on_destroy);
    }
    wnd_instance create() override;
private:
    std::string title_;
    size size_;
    WNDCLASSEX wcex_;
    std::string class_;
protected:
    void on_destroy() {
        ::PostQuitMessage(0);
    }            
};
~~~

And that's it for Ms Windows message routing. Let's now implement the same
logic for the GTK.

### On Unix

We have extended a lambda init function to the `signal` class with a reason.
In GTK there is no global window procedure. Instead GTK already implements 
its own signals. What we want to do is subscribe to the underlying GTK signal
and forward them to our signals. But which signals do we want to listen to? 
It would be quite expensive for every window to subscribe to all possible 
signals. That is why we added the lambda init function. This init function
is called on the first connect to our signal. At that moment it subscribes
to the underlying GTK signal, on a need-to-subscribe basis.

A definition of the signal is a bit different on Unix.

~~~cpp
#if _WIN32
        signal<> destroyed;
#elif __unix__
        signal<> destroyed{ [this]() { ::g_signal_connect(G_OBJECT(instance()), "destroy", G_CALLBACK(wnd::global_gtk_destroy), this); } };
#endif
~~~

We pass a GTK signal subscription function as lambda to our `destroyed` signal. 
Note that in the subscription function we pass `this` pointer. That it because GTK 
signals, just like MS Windows window procedures, are global functions. When the 
`wnd::global_gtk_destroy()` function is called it must know to which window it needs
to forwards this message. Luckily it receives an extra parameter, which we pass
in the `::g_signal_connect()` function. And that parameter is ... `this`.

Forward the call is trivial.

~~~cpp
static void global_gtk_destroy(GtkWidget* widget, gpointer data) {
    wnd* w = reinterpret_cast<wnd*>(data);
    w->destroyed.emit();
}
~~~cpp

And, finally, the `signal::connect()` function uses the lambda init function to 
subscribe upon first connect to our signal.  

~~~cpp
int connect(std::function<bool(Args...)> const& slot) const {
    if (!initialized_ && init_ != nullptr) { init_(); initialized_ = true; }
    slots_.insert(std::make_pair(++current_id_, slot));
    return current_id_;
}
~~~

All that is left is to enrich our `app_wnd::on_destroy()` handler.

~~~cpp
bool app_wnd::on_destroy() {
#if _WIN32
    ::PostQuitMessage(0);
#elif __unix__
    ::gtk_main_quit(); // New!
#endif
    return true; // Message processed.
}   
~~~

And we're done with message routing. 

## A time to paint!

To every thing there is a season, and a time to every purpose under the heaven.
And now it is time for a bit of instant gratification. So let's create a simple
class called artist (actually it should be called dumb artist!) that only know
how to paint rectangles. It's not much - but it is a proof of concept.

### Basic structures

Let's define three very basic structures: a color (RGB), a point and a rectangle.

~~~cpp
struct color {
        byte r;
        byte g;
        byte b;
        byte a;
    };

    struct pt {
        union { coord left; coord x; };
        union { coord top; coord y; };
    };

    struct rct {
        union { coord left; coord x; coord x1; };
        union { coord top; coord y;  coord y1; };
        union { coord width; coord w; };
        union { coord height; coord h; };
        coord x2() { return left + width; }
        coord y2() { return top + height; }
    };
~~~

### Artist

( ... to be continued ... )