# Let's Paint!

Hello again. In previous two lessons we learned how to create
platform independent application class and window class. In this
chapter we are going to process some event's and paint some
lines to its surface.

Our starting point will be, as usual, the code that we wrote 
in previous lesson. And our target code (which must compile and
work after this lesson) will be this.

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
great artists steal!" Not Steve Jobs, and not Pablo Picasso. 
We are going to be good artists, and reuse an existing [signals
and slots implementation](https://schneegans.github.io/tutorials/2015/09/20/signal-slot)  
by Simon Schneegans. His code follows the nice philosophy: it is short, 
and utilizes modern C++11 (variadic templates).

So without any further talk here is the stripped version of his code.

~~~cpp
template <typename... Args>
class signal {
public:
    signal() : current_id_(0) {}
    template <typename T> int connect(T* inst, void (T::* func)(Args...)) {
        return connect([=](Args... args) {
            (inst->*func)(args...);
            });
    }

    template <typename T> int connect(T* inst, void (T::* func)(Args...) const) {
        return connect([=](Args... args) {
            (inst->*func)(args...);
            });
    }

    int connect(std::function<void(Args...)> const& slot) const {
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
        for (auto const& it : slots_) {
            it.second(std::forward<Args>(p)...);
        }
    }
private:
    mutable std::map<int, std::function<void(Args...)>> slots_;
    mutable int current_id_;
};
~~~

This code enables us to create a signal on the transmitter's side,
and listen (connect) to it on the receiver's side. 

We create a signal like this

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

we raise a signal by calling `paint.emit`, and we listen to it 
inside our window like this

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

Next we need to catch native events from both platforms (Win32 and GTK),
and rewire them to signals. And we're done.

## Catching native events

### On Windows

#### The window procedure

Every window on Windows has a window procedure. This procedure is called when
by the main message loop (from prevous lesson) when the window receives a message.
On Windows the message procedure is attached to the window when we register
the window class.

We already have code that creates window class inside our `nice.hpp`, in the
create function for the window. The member `wcex_.lpfnWndProc points to 
window procedure that receives the messages. And currently window procedure
just handles the *destroy window* message.

~~~cpp
wnd_instance app_wnd::create() {
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

Every window needs a window procedure. Not only that - every window class (i.e. an application window, 
a button, a text box) needs a different window procedure. Hence it makes sense to move it to 
the base `wnd` class instead of `app_wnd` class.

But wait! The window procedure must be specific to an instance of window class, and the `global_wnd_proc` 
is static - so there can be only one for all derived windows.

To mitigate this we must implement implement a mechanism similar to a railroad switch. If 
`global_wnd_proc` will receive all messages for all `wnd` derived windows, it needs to 
forward each message to its' correct local instance.

#### The instance specific window procedure

To implement instance specific window procedure our `global_wnd_proc` examines the very first 
window message for particular window, which is (according to documentation!) the `WM_NCCREATE`. 
This message brings with it a special structure `LPCREATESTRUCT` and a member of this structure 
called `lpCreateParams`. We can set the value of this member when we call the `::CreateWindow` 
in our `create()` procedure.

This is the code fragment call from the `app_wnd::create()`. Note that last parameter 
is `this` and because it is inside a member function of `wnd` derived class, it passes 
pointer to C++ window class instance.

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

In our global window procedure we must catch the `WM_NCCREATE' and read this value. 
We then store it to internal Win32 window structure using the `::SetWindowLongPtr` API
function which allows us to attach a variable of type `long` to the native Win32 
window structure. From that moment on the instance of C++ class is forever linked with 
an instance of Win32 window structure. 

Here's how we do it.

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
        return ::DefWindowProc(instance(), msg, p1, p2);
    }
};
~~~

From now on every `wnd` derived class can override `local_wnd_proc` and implement
class specific behaviour for a window.

#### Implement signals, common to all windows

Some window messages are class specific, and some are sent to all windows.
Examples of messages sent to all windows are: 
 * message sent to window when it is created,
 * message sent to window when it is destroyed,
 * message sent to window when window needs to repaint itself,
 * message sent to window when mouse is moved across the surface.
It makes sense to wire messages, that are common to all windows, 
to their respective windows, inside the base `wnd` class. So let's do it
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

We now have Window code that rewires window destruction to the `destroyed` signal.
So in the `app_wnd` we can close application when this signal is emitted. 

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

Congratulations. You read through it. We have successfully implemented window message
routing for MS Windows.

### On Unix