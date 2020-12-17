 # Welcome to nice
Nice is a modern C++ micro framework for building desktop applications
that started as an excercise in modern C++ to refresh my skills. 

> It is an experimental development; unstable, and poorly documented. 
> It will live up to expectations. Just not now. Thank you for your patience.

The philosophy of nice is to:
 * keep it simple; if it is not, refactor it until it is,
 * make it micro; an individual must to be able to fully understand it,
 * hide native API complexities and expose nice interfaces: hence the name
 * enable creating derived classes on top of existing classes
 * use best of C++20
 * use single header
 * support native C++ multithreading 
 * support modern layout managers
 * superfast, and supersmall; load instantly and use kilobytes, not megabytes!
 * single exe
 * be multiplatform.

# Hello nice
Here's the Hello World application in nice:
~~~
#include "nice.hpp"

using namespace nice;

void program()
{
    app::run(app_wnd("Hello World!"));
}
~~~

And here's the Hello Paint application in nice:
~~~
#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello paint!") {
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    // Paint handler, draws rectangle.
    void on_paint(const artist& a) {
        a.draw_rect({ 255,0,0 }, { 10,10,200,100 });
    }
};

void program()
{
    app::run(main_wnd());
}
~~~

# Compiling

## Windows

Install Visual Studio 64bit compilers, and cmake. 

~~~
cmake .
make
~~~

## Linux

Coming soon.


# Status

## Done
 * proof of concept: hello world
 * transformed into single header library
 * ms windows binding
 * mapping window messages to C++ signals
 * basic paint proof of concept
 * abstracting drawing primitives
 * standards for pointer use!
 * refactoring no 1
 * standard controls POC (button, text edit)
 * exceptions
 * fonts poc
 * refactoring no 2 (remove shared_ptr overkills)

## Implementing
 * basic layout manager POC
 * scribble app
 * calculator app
 * refactoring no 3

## Planning
 * custom controls
 * paint app
 * refactoring no 4
 * multithreading operations
 * gtk+ binding


# How to be nice

Nice builds around several idioms. 

## Two phase initialization

Most windows resources (handles, etc) are initially created as 
C++ objects, and then a system resource is created and attached
to them. This two phase initialization is implemeted using 
a combination of a base class called `resource` and  
lazy evaluation.

## Class properties

For class properties a get and a set overloaded functions are created 
with the same name. The get function takes no arguments, and returns a 
value (of property type). And the set function has no return type, 
and accepts one argument (of property type). 

Property example:
~~~
int width() const; // Get.
void width(int value); // Set.
~~~

## Exceptions

Throw `nice_exception` using the `throw_ex` macro. This macro expands to
include `__FILE__`, `__FUNCTION__`, and `__LINE__` into the exception when c
compiled with `_DEBUG` flag.

# Bumps on the road

The mistakes we made and lessons we took on the way...

## To Wayland or not to Wayland?

Not to Wayland, at thist time.

Wayland is a technology for drawing things. It has no widgets. If ~nice~ had
been implemented on top of Wayland, it would have to re-implement many features, 
that are already part of toolkits, such as GTK+ or QT.

## Fluent interface

Fluent interface was abandoned. C++ is simply not there yet. A lot of magic with 
templates has been tested, starting with [CRPT] (https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern), 
and ending with [our own pattern](https://stackoverflow.com/questions/62995665/c-fluent-interface). 
However, results were always too complicated and unreadable to include it into a library
which strives to simplify desktop programming.

## Friend "native" functions

All classes have some native members, which should be visible in derived classes.
An early effort to separate native code from the pure nice classes (for Microsoft Windows)
featured a lot of friend functions, and a standard C++ map of all windows that redirects 
window messages (from window procedures to member functions).

After initial refactoring friend functions were replaced by static member functions, and 
the map mechanism with storing window class in window structure.


# How does it work?

## How and when when is a window created?

Window creation is a two stage process. In first stage a C++ object is created, 
but it has no native window attached. In second stage a native window
is created. Second stage is only called when the native window is needed for
the first time.

To implement this each `wnd` is derived from `native_wnd`. Native window is a structure 
that encapsulates basic Win32 API calls and structures. It window exposes native window
handle as a property named `id()`. When you try to access this property for the first time, 
the native window is created by calling the `create()` pure virtual function. By 
overriding this function a custom creation code can be provided for the inherited class.

~~~
typedef HWND wnd_id;
...
class native_wnd {
public:
    // Ctor.
    native_wnd() : hwnd_(NULL) {}
    virtual ~native_wnd() { if (hwnd_ != NULL) destroy(); }

    virtual wnd_id create() = 0;
    virtual void destroy() {};

    // Properties.
    virtual wnd_id id() { 
        if (hwnd_ == NULL)
            hwnd_ = create();
        return hwnd_; 
    }
    virtual void id(wnd_id id) { hwnd_ = id; }
private:
    // Window handle.
    HWND hwnd_;
};
~~~

Code fragment above shows main parts of native_wnd that collaborate to implement
creation and destruction of window.

> Propery `id()` hides native `hwnd_` handle. Only `native_wnd` can use the handle directly
> to avoid unnecessary creation of window. All derived classes use the `id()` get accessor.

TODO: But shouldn't you avoid calling virtual functions from destructors, even if virtual?

## Static application class

Different environments have different start up functions. For example, 
MS Windows uses WinMain, but X Windows uses standard C/C++ main.

To unify start up procedure `nice`:
 * has standard application entry point. you must provide 
   function `program()`.
 * declares a static application class which is populated by the start up 
   function.

~~~
class app
{
public:
    static app_id id() const;
    static void id(app_id id);
    static int ret_code() const;
    static void run(std::shared_ptr<app_wnd> w);
};
~~~

Each application has:
 * unique identifier id() 
 * return code ret_code(), and
 * a run function taking main window as the only argument.

## Operating system types

Following operating system types are declared for each environment. 

 * app_id. This is a value that uniquely identifies the application.
 * wnd_id. A value that uniquely identifies window.
 * msg_id. A value that uniquely identifies window message.
 * par1. Each message has two parameters. This it type for 
   the first parameter.
 * par2. -"-. This is type for the second parameter.
 * result. A value that uniquely identifies system result / error.
 * coord. Screen coordinate.
 * byte. 8 bit integer.

Here is an example of these types for Microsoft Windows.

~~~
typedef HINSTANCE app_id;
typedef HWND wnd_id;
typedef UINT msg_id;
typedef WPARAM par1;
typedef LPARAM par2;
typedef LRESULT result;
typedef LONG coord; 
typedef BYTE byte;
~~~

## Common structures

* size size
* rct rectangle
* pt point
* color rgba color structure


## Window hierarchy

All windows are derived from `wnd`. This one, itself, is derived from
`native_wnd`. Parent windows (main_wnd, dialog_wnd) are derived from `parent_wnd`,
and child windows (button, text_edit, label) are derived from `child_wnd`.

### Adding child windows

All child windows are intially created as message only windows. When added
to the parent by calling `add(std::shared_ptr<child_wnd> child)` windows are
reparented.


# Links

Following were particularly useful when developing nice.

How to handle windows state.
 * https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
 * https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992

Windows Fonts
 * http://winprog.org/tutorial/fonts.html

User defined literals
 * https://akrzemi1.wordpress.com/2012/08/12/user-defined-literals-part-i/


# Brainstorming

## Layout manager

Each `parent_wnd` has a `layout_manager` i.e. a default absolute positioning
layout manager. This manager listens to window resize events, and detects 
adding and removing child controls. It then relayouts all child controls.

Derived layout managers use different logic to relayout child controls.

How to handle various parameters for different layout managers (i.e. row and
column for grid layout manager, coords. for default layout manager)? Easy,
by separating placeholder and control. You create a placeholder 1 in layout and
add control to it. Placeholder has all special settings, not the control.

There has to be a "next placeholder" setting (default setting) for automatic
layout managers.

What about nesting layout managers?

## Fonts

Using C++11 literas for em, px, and other units would be interesting.
Font size would be written as 10_px or 1.5_em.

## Intellisense

::create disables intellisense, and hides real error location. That's really bad!
Got to do something about it.

## Usage of shared_ptr

Using shared_ptr causes a lot of troubles. One being inability to use operators
in a clean manner. Why couldn't we simply pass windows by value / const reference.

## Plugins

Should we include plugins into GUI library?

http://www.cplusplus.com/articles/48TbqMoL/ 
https://sourcey.com/articles/building-a-simple-cpp-cross-platform-plugin-system

The answer seems to be: NO.

## Properties

Here is a pattern for properties in nice.

~~~
#include <iostream>
#include <functional>

template<typename T>
class property {
public:
    property(
        std::function<void(T)> setter,
        std::function<T()> getter) :
         setter_(setter),getter_(getter) { }
    operator T() const { return getter_(); }
    property<T>& operator= (const T &value) { setter_(value); return *this; }
    T& value() { return value_; }
private:
    std::function<void(T)> setter_;
    std::function<T()> getter_;
    T value_;
};

class person {
public:
property<int> age {
    [this] (int x) { this->age.value()=x; }, // Setter ... yes you can.
    [this] () -> int {  return this->age.value(); } // Getter.
    };
};

int main() {
    person p;
    p.age=10; // Call setter.
    std::cout << p.age; // Call getter.
}
~~~

# Generic Layout Manager

Our goal is a lightweight layout manager that can only do one thing: 
layout its children. You populate the layout manager using the <<
operator, and it can host child layout managers.

Basic idea is

~~~
#include <vector>
#include <iostream>

struct rct {
public:
    int x,y,w,h;
};

struct layout_pane {
public:
    // Apply layout to child panes.
    virtual void apply(rct r) {
        std::cout << "layout_pane(" 
            << r.x << "," << r.y << "," 
            << r.w << "," << r.h
            << ")"<< std::endl;
        for (layout_pane& p : children_) 
                p.apply(r); // Default layout maximizes children.
    }
    
    virtual layout_pane& operator<<(const layout_pane& p) // Add child pane.
    {
        children_.push_back(p);
        return *this;
    }
protected:
    std::vector<layout_pane> children_; // Child panes.
};

// Structure children vertically,
struct vertical_layout_pane : public layout_pane {
public:
    // Apply layout to child panes.
    virtual void apply(rct r) override {
        std::cout << "vertical_layout_pane("
            << r.x << "," << r.y << "," 
            << r.w << "," << r.h
            << ")"<< std::endl;
        auto n=children_.size();
        if (n>0) {
            int y=0, y_step=r.h/n;
            for (layout_pane& p : children_) {
                p.apply({r.x,y,r.w,y_step}); // Default layout maximizes children.
                y+=y_step;
            }
        }
    }
};

// Structure children horizontally.
struct horizontal_layout_pane : public layout_pane {
public:
    // Apply layout to child panes.
    virtual void apply(rct r) override {
        std::cout << "horizontal_layout_pane(" 
            << r.x << "," << r.y << "," 
            << r.w << "," << r.h
            << ")"<< std::endl;
        auto n=children_.size();
        if (n>0) {
            int x=0, x_step=r.w/n;
            for (layout_pane& p : children_) {
                p.apply({x,r.y,x_step,r.h}); // Default layout maximizes children.
                x+=x_step;
            }
        }
    }
};


int main() {
    auto l = vertical_layout_pane() << vertical_layout_pane(); // << horizontal_layout_pane() << layout_pane();
        //<< ( horizontal_layout_pane() << layout_pane() )
        //<< ( horizontal_layout_pane() << layout_pane() << layout_pane() );

    l.apply({0,0,100,100});

    return 0;
}
~~~ 

This will create



# Further reading...

http://ptomato.name/advanced-gtk-techniques/html/introduction.html
https://www.linuxtopia.org/online_books/gui_toolkit_guides/gtk+_gnome_application_development/advanced.html
