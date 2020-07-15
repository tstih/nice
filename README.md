# Welcome to nice
Nice is an experimental modern C++ library for building graphical user interfaces. 

> This is early development. Sometimes things don't compile and are documented poorly. 
> nice will live up to expectations, just not now. Thank you for your patience.

Hello world in nice:
~~~
#include "nice.hpp"

void program()
{
    nice::app::run(app::run(
        std::unique_ptr<app_wnd>(
            ::create<app_wnd>("Hello world!")
        )
    );
}
~~~

Question? Why using shared pointer when I can use pass by value and
move semantics?

Hello paint in nice:
~~~
#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello paint!"), ok(nullptr) {
        
        // Subscribe to events.
        created.connect(this, &main_wnd::on_created);
        paint.connect(this, &main_wnd::on_paint);
        
    }
private:

    // OK button.
    std::unique_ptr<button> ok;
    void on_created() {
        // Create child controls.
        ok = std::unique_ptr<button>(
            ::create<button>(id(), "OK", rct{ 100,100,196,126 })
            ->text("CLOSE")
            );
    }

    // Paint handler, draws rectangle.
    void on_paint(std::shared_ptr<artist> a) {
        a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
    }
};

void program()
{
    app::run(
        std::unique_ptr<main_wnd>(
            ::create<main_wnd>()
        )
    );
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

UPDATE: Linux support temporary removed. Coming back soon.

nice depends on GTK3 library. Install it to your machine if
not already installed.

`sudo apt install libgtk-3-dev`

Then do 

~~~
cmake .
make
~~~

# The Story

It started as an excercise in modern C++. I needed a simple 
library for programming Microsoft Windows desktop applications. 

The philosophy of nice is to
 * hide native complexity and expose nice C++0x interface, hence the name
 * enable creating derived classes on top of existing classes
 * use single header
 * be multiplatform

First naive version for Microsoft Windows featured a lot of friend functions to
hide native implementation. A simple map structure was used to redirect window 
messages from window procedures to member functions.

After few refactoring phases, friend functions were replaced by
static member function.

Afterwards I hit some MS Windows design fundamentals, such as that
CreateWindow function already sends and posts messages before window
handle is even returned by the functions. So I remodelled the 
message chain according to these two sources:
 * https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
 * https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992

# Status

## Done
 * proof of concept: hello world
 * transformed into single header library
 * ms windows binding
 * mapping window messages to C++ signals
 * basic paint proof of concept
 * abstracting drawing primitives

## Implementing
 * gtk+ binding
 * standards for pointer use!

## Planning
 * refactoring no 1
 * exceptions
 * fluent interface
 * refactoring no 2
 * standard controls (buttons, scrollbars, text edit)

# Dilemmas

## Internal access to members

We are trying to hide platform specific class members. One way to do this is to
use friend classes for private access, but this causes problems to derived classes
and can't be a long term solution.

UPDATE: I removed friend functions and restructured the code to provide nice class
members.

## To Wayland or not to Wayland?

Wayland is a technology for drawing things. It has no widgets. If ~nice~ is 
implemented on top of Wayland, it will have to re-implement many features, 
that are already part of toolkits, such as GTK+ or QT.

On the other side, such library will be portable to embedded systems with 
only basic 2D drawing functions.

## Base window function

Has two pure virtuals that prevent it to be passed to other funtions. 
Remove the two pure virtuals and restructure code.

# How to be nice

Nice builds around several idioms. 

## Static application class

Different environments have different start up functions. For example, 
MS Windows uses WinMain, but X Windows uses standard C/C++ main.

To unify start up procedure ~nice~:
 * has standard application entry point. you must provide 
   function ~program()~.
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

# Idioms

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

## Operating system types

Following operating system types are declared for each environment. 

 * app_id. This is a value that uniquely identifies the application.
 * wnd_id. A value that uniquely identifies window.
 * msg_id. A value that uniquely identifies window message.
 * par1. Each message has two parameters. This it type for 
   the first parameter.
 * par2. -"-. This is type for the second parameter.
 * result. A value that uniquely identifies system result / error.

Here is an example of these types for Microsoft Windows.

~~~
typedef HINSTANCE app_id;
typedef HWND wnd_id;
typedef UINT msg_id;
typedef WPARAM par1;
typedef LPARAM par2;
typedef LRESULT result;
~~~

## Common structures

* size size
* rct rectangle
* pt point

## Fluent interface

The two phase window creation process poses a challenge to the fluent interface.

Imagine a situation where you set the title of a window to some value before 
creating a window. The class must store this value and use it when creating a 
window. But if a change is made after creating a window, window title must be updated.

Hence in case of fluent interface the implementation of the setter function for the title 
will behave differently depending on the call order. 

Here's one way to handle it.

~~~
void wnd::text(std::string t)
{
    text_ = t;
    if (id()) // if window exists
        ::SetWindowText(id(), t.data());
}
~~~

## Tips and tricks used

Obtaining applicaiton instance ~(HINSTANCE)GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)~