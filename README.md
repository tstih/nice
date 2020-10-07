# Welcome to nice
Nice is a modern C++ library for building graphical user interfaces. 

> This is an experimental development; unstable, and poorly documented. 
> It will live up to expectations. Just not now. Thank you for your patience.

It started as an excercise in modern C++ to refresh my skills. 

The philosophy of nice is to:
 * hide native API complexities and expose nice C++17 interface: hence the name
 * enable creating derived classes on top of existing classes
 * use single header
 * be multiplatform
 * support native C++ multithreading 
 * introduce layout managers


# Hello nice
Here's the Hello World application in nice:
~~~
#include "nice.hpp"

using namespace nice;

void program()
{
    app::run(::create<app_wnd>("Hello world!"));
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
    void on_paint(std::shared_ptr<artist> a) {
        a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
    }
};

void program()
{
    app::run(::create<main_wnd>());
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

## Implementing
 * standard controls (button, text edit, etc.)

## Planning
 * exceptions
 * basic layout manager
 * refactoring no 2
 * gtk+ binding
 * custom controls
 * refactoring no 3
 * multithreading operations


# How to be nice

Nice builds around several idioms. 

## Using shared pointers for windows

What we do here is [against the rules of passing smart pointers](https://www.modernescpp.com/index.php/c-core-guidelines-passing-smart-pointer)
but it works well for us. We use `std::shared_ptr` for window resources. 

## Two phase initialization

Most windows resources (handles, etc) are initially created as 
C++ objects, and then a system resource is created and attached
to them. This two phase initialization is implemeted using smart
pointers with custom deleters.

Anything that supports two phase initialization must implement
`create()` and `destroy()` functions. You should implement acquiring 
and releasing system resources in these two functions.

Then use ~::create()~ global function to create these classes.

~~~
auto ok = ::create<button>("OK", rct{ 100,100,196,126 });
~~~

So how does this `::create()` work? Like this:

~~~
template <class T, typename... A>
std::shared_ptr<T> create(A... args)
{
    // Create a shared pointer with a custom deleter.
    std::shared_ptr<T> ptr(new T(args...), [](T* p) { p->destroy();  delete p; });
    ptr->create();
    return ptr;
}
~~~

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


# Links

Following were particularly useful when developing nice.

How to handle windows state.
 * https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
 * https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992