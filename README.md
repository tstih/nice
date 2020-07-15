# Welcome to nice
Nice is a modern C++ library for building graphical user interfaces. 

> This is an experimental development; unstable, and poorly documented. 
> It will live up to expectations. Just not now. Thank you for your patience.

# The Story

It started as an excercise in modern C++ to refresh my skills. 

The philosophy of nice is to:
 * hide native API complexities and expose nice C++17 interface: hence the name
 * enable creating derived classes on top of existing classes
 * use single header
 * be multiplatform
 * use fluent interface

First naive version for Microsoft Windows featured a lot of friend functions,
a simple standard map to redirect window messages (from window procedures to 
member functions), and a lot of shared_ptrs.

After initial refactoring friend functions were replaced by static member 
functions, and the map mechanism with storing window class in window structure.

I recently upgraded the code to respect some MS Windows design fundamentals, such as 
that CreateWindow function sends and posts messages to window before the
CreateWindow or CreateWindowEx function even returns window handle. 
I remodelled the message chain according to these two sources:
 * https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
 * https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992

In addition to that I created a pattern for two phase initialization of window
classes, and introduced fluent interface.

Here's the Hello World application in nice:
~~~
#include "nice.hpp"

using namespace nice;

void program()
{
    app::run(
        std::unique_ptr<app_wnd>(
            ::create<app_wnd>("Hello world!")
        )
    );
}
~~~

And here's the Hello Paint application in nice:
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

**UPDATE: Linux support temporary removed. Coming back soon.**

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
 * standards for pointer use!
 * fluent interface
 * refactoring no 1

## Implementing
 * standard controls (buttons, scrollbars, text edit)

## Planning
 * exceptions
 * refactoring no 2
 * gtk+ binding

# Dilemmas

## Internal access to members

We are trying to hide platform specific class members. One way to do this is to
use friend classes for private access, but this causes problems to derived classes
and can't be a long term solution.

I removed friend functions and restructured the code to provide nice class
members. In addition I moved most native logic to native_wnd. Let's see if I can
get everything there.

## To Wayland or not to Wayland?

Wayland is a technology for drawing things. It has no widgets. If ~nice~ is 
implemented on top of Wayland, it will have to re-implement many features, 
that are already part of toolkits, such as GTK+ or QT.

On the other side, such library will be portable to embedded systems with 
only basic 2D drawing functions.

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