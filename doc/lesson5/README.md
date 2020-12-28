# It's nice to have children

They say children are our greatest treasure. This lesson 
is dedicated to managing children and keeping them in order.
In it we will create a layout manager that will handle two 
child controls (a button and a label).

# Extend the window class

So far our event handling was more of a proof of concept
then a serious undertaking. Before embarking on this quest, 
let's enrich our core window by adding:
 * ability to read and write window size,
 * ability to read and write window position,
 * resize event handler,  
 * ability to trigger naive window repaint, and
 * event handlers for keyboard and mouse,
 * naive text output.

But only after we get familiar with a few patterns.

# The property pattern

To add some of the required features to the window class,
let's add *proper properties*. They will beave just like
variables; but when read from or write to - they'll call 
functions to accomplish the task. 

We will be able to write

~~~cpp
w.title="Hello!";
w.wsize={640,400};
~~~

instead of

~~~cpp
w.set_title("Hello!");
w.set_wsize({640,400});
~~~

Here's the class that enables us to define a property.

~~~cpp
template<typename T>
class property {
public:
    property(
        std::function<void(T)> setter,
        std::function<T()> getter) :
        setter_(setter),getter_(getter) { }
    operator T() const { return getter_(); }
    property<T>& operator= (const T &value) { setter_(value); return *this; }
private:
    std::function<void(T)> setter_;
    std::function<T()> getter_;
};
~~~

If you'd like to enable overriding a property, it should 
call a protected virtual setter and getter. Let's respect 
the following convention for these functions: a protected 
setter and getter function shall be called the same as
the property variable, but prefixed with a set_ or a get_. 
For example `set_size` and `get_size` for property `size`. 

If this theory sounds a bit complex, here's how to do it in
practice.

~~~cpp
class wnd : public resource<wnd_instance> {
    // ... code omitted ...
public:
    property<std::string> title {
        [this] (std::string s) { this->set_title(s); },
        [this] () -> std::string {  return this->get_title(); }
    };
};
protected:
    virtual std::string get_title() {} // Implement it!
    virtual void set_title(std::string s) {} // Implement it!
    // ... code omitted ...
}
~~~

So for each property - we write plain getter and setter, and then 
simply add the variable with lambdas that call these functions.
If we want to override it, we override the protected getter and/or setter.

 > In future, we may refactor the `property` class to derivate 
 > from a `read_property` and `write_property` classes, each implementing
 > half of functionality, read only and write only properties.

# The unit pattern

Our layout manager, and text output will both require 
handling different units, such as percents, pixels, ems, etc. 
A nice solution to this is using [user-defined literals](https://www.geeksforgeeks.org/user-defined-literals-cpp/), which have been around since C++11. 
User defined literals will allow us to write quantities, such as `50_pc`, `200_px`,
or `1.5_em`.

 > Why the underscore? Wouldn't it look better if we define 
 > without it ie.. `50pc`,`200px`, and `1.5em`? It most
 > definitely would. But to assure that our software works
 > with any future standard library, literals without underscores
 > should only be defined by the standard library.

For now we'll define percent and pixel units. 

~~~cpp
class percent
{
    double percent_;

public:
    class pc {};
    explicit constexpr percent(pc, double dpc) : percent_{ dpc } {}
};

constexpr percent operator "" _pc(long double dpc)
{
    return percent{ percent::pc{}, static_cast<double>(dpc) };
}

class pixel
{
    int pixel_;

public:
    class px {};
    explicit constexpr pixel(px, int ipx) : pixel_{ ipx } {}
    int value() { return pixel_; }
};

constexpr pixel operator "" _px(unsigned long long ipx)
{
    return pixel{ pixel::px{}, static_cast<int>(ipx) };
}
~~~

# Handling window properties 

Time to implement the size property, and the resized event.

We begin by adding a new signal, a getter function, a setter function,
and a property to our wnd class.

~~~cpp
class wnd : public resource<wnd_instance> {
    // ... code omitted ...
public:
    signal<> resized;
    property<size> wsize {
        [this] (size sz) { this->set_wsize(sz); },
        [this] () -> size {  return this->get_wsize(); }
    };
};
protected:
    virtual std::string get_wsize(); 
    virtual void set_wsize(std::string s); 
    // ... code omitted ...
}
~~~

 > In this lesson we will **not** be showing how to implement  all new 
 > properties (`wsize`, `title`, and `location`) and their events.  
 > One example is enough. If you want to understand others - read the code. 
 
# Handling more events

Let's now add mouse, keyboard, and a few basic windows events (i.e. resize). 
We are going to start with very simple structures for these events, and evolve 
them as we go. The signals to add are: `key_press`, `key_release`, 
`mouse_down`, `mouse_up`, `mouse_move`, and `resized`.

## New events

### Mouse events

Following is the data structure for mouse events that we plan to pass with mouse
signals. It contains current mouse position, the status of three mouse buttons,
and the status of shift, control, and alt keys.

~~~cpp
struct mouse_info {
    pt location;
    bool left_button;
    bool middle_button;
    bool right_button;
    bool ctrl;
    bool shift;
    bool alt;
};

class wnd : public resource<wnd_insance> {
    // ... code omitted ...
    signal<const mouse_info&> mouse_move;
    signal<const mouse_info&> mouse_down;
    signal<const mouse_info&> mouse_up;
    // ... code omitted ...
};
~~~

To emit these signals, we'll simply extend local window procedure for the `wnd`
class for each platform. As we already explained event routing in earlier sessions,
here's the code fragment routing mouse events on the MS Windows platform. 

~~~cpp
case WM_MOUSEMOVE:
case WM_LBUTTONDOWN:
case WM_MBUTTONDOWN:
case WM_RBUTTONDOWN:
case WM_LBUTTONUP:
case WM_MBUTTONUP:
case WM_RBUTTONUP:
    {
    // Populate the mouse info structure.
    mouse_info mi = {
        {GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam)}, // point
        wparam & MK_LBUTTON,
        wparam & MK_MBUTTON,
        wparam & MK_RBUTTON,
        wparam & MK_CONTROL,
        wparam & MK_SHIFT,
        ::GetKeyState((VK_RMENU) & 0x8000) || ::GetKeyState((VK_LMENU) & 0x8000) // Right ALT or left ALT..doesn't work!!!
    };
    if (msg == WM_MOUSEMOVE)
        mouse_move.emit(mi);
    else if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN)
        mouse_down.emit(mi);
    else
        mouse_up.emit(mi);
    }
    break;
~~~

# Multiplatform Scrible

Excellent. We now have enough functionality on window to implement **the
Scrible**. 

~~~cpp
#include <vector>
#include "nice.hpp"

using namespace ni;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Scrible", { 800,600 })
    {
        drawing_ = false;

        paint.connect(this, &main_wnd::on_paint);
        mouse_down.connect(this, &main_wnd::on_mouse_down);
        mouse_up.connect(this, &main_wnd::on_mouse_up);
        mouse_move.connect(this, &main_wnd::on_mouse_move);
    }
private:
    std::vector<std::vector<pt>> strokes_;
    bool drawing_;

    bool on_paint(const artist& a) {
        // No strokes to draw yet?
        if (strokes_.size() == 0) return true;
        for (auto s : strokes_) {
            auto iter = s.begin(); // First point.
            auto prev = *iter;
            for (advance(iter, 1); iter != s.end(); ++iter)
            {
                auto p = *iter;
                a.draw_line({ 0,0,0 }, prev, p);
                prev = p;
            }
        }
        return true;
    }

    bool on_mouse_down(const mouse_info& mi) {
        drawing_ = true;
        strokes_.push_back(std::vector<pt>());
        strokes_.back().push_back(mi.location);
        return true;
    }

    bool on_mouse_move(const mouse_info& mi) {
        if (drawing_) {
            strokes_.back().push_back(mi.location);
            repaint();
        }
        return true;
    }

    bool on_mouse_up(const mouse_info& mi) {
        drawing_ = false;
        strokes_.back().push_back(mi.location);
        repaint();
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}
~~~

