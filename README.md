# Welcome to nice
Nice is an experimental modern C++ library for building graphical user interfaces. 

> This is early development. Sometimes things don't compile and are documented poorly. 
> nice will live up to expectations, just not now. Thank you for your patience.

Hello world in nice:
~~~
#include "nice.hpp"

void program()
{
    nice::app::run(std::make_shared<nice::app_wnd>("Hello world!"));
}
~~~

Hello paint in nice:
~~~
#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello paint!") {
        // Subscribe to event.
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    void on_paint(std::shared_ptr<artist> a) const {
        a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
    }
};

void program()
{
    nice::app::run(
        std::static_pointer_cast<nice::app_wnd>(
            std::make_shared<main_wnd>()
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

nice depends on GTK3 library. Install it to your machine if
not already installed.

`sudo apt install libgtk-3-dev`

Then do 

~~~
cmake .
make
~~~

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

## To Wayland or not to Wayland?

Wayland is draw-only technology, no widgets. Would it be easier to implement on
top of GTK instead? If not, I'll have to reimplement plenty. But, on the other side,
I'll create library, capable of begin ported to embedded systems with only basic 2D
drawing functions.

# How to be nice

Nice builds around several idioms. 

## Static application class

Different environments have different start up functions. For example, MS Windows uses WinMain, but X Windows uses standard C/C++ main.

To unify start up procedure nice:
 * unifies application entry point, you must provide
   function program().
 * declares a static application class which is populated
   by the start up function.

~~~
class app
{
public:
    static app_id id();
    static void id(app_id id);
    static int ret_code();
    static void run(std::shared_ptr<app_wnd> w);
};
~~~

Each application has:
 * unique identifier id() 
 * return code ret_code(), and
 * a run function taking main window as the only argument.

# Idioms

## Class properties

For class properties a get and a set overloaded functions are created with the same name. The get function takes no arguments, and returns a value (of property type). And the set function has no return type, and accepts one argument (of property type). 

Property example:
~~~
int width(); // Get.
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

The problem of before window creation and after window creation functions...