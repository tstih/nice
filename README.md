 # Welcome to nice

by Tomaz Stih

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

~~~cpp
#include "nice.hpp"

using namespace ni;

void program()
{
    app::run(app_wnd("Hello World!"));
}
~~~

And here's the Hello Paint application in nice:

~~~cpp
#include "nice.hpp"

using namespace ni;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello paint!") {
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    // Paint handler, draws rectangle.
    void on_paint(const artist& a) {
        a.draw_rect({ 255,0,0 }, { 10,10,100,100 });
    }
};

void program()
{
    app::run(main_wnd());
}
~~~


# Compiling

## Windows

Install Visual Studio 64bit compilers, and cmake it.

## Linux

Unpack it. Then go to the target folder and do

~~~
cd build
cmake ..
make
~~~


# Status

## Done
 * proof of concept: hello world
 * transformed into single header library
 * ms windows binding
 * unis binding (GTK3)
 * mapping window messages to C++ signals

## Implementing
 * basic paint proof of concept
 * basic layout manager POC
 * scribble app
 * calculator app
 * refactoring no 3

## Planning
 * custom controls
 * paint app
 * refactoring no 4
 * multithreading operations


# Nice tutorial

Part 1: [Nice application.](http://github/tstih/nice/doc/lesson1)

Part 2: [Basic windows.](http://github/tstih/nice/doc/lesson2)

Part 3: [Window message routing, and painting.](http://github/tstih/nice/doc/lesson3)

Part 4: Child windows and layouting

Part 5: Custom Controls

Part 6: It's all about being nice


# Bumps on the road

The mistakes we made and lessons we took on the way.

## To Wayland or not to Wayland?

Perhaps one day we will Wayland, but at this moment we won't. Wayland is a technology 
for drawing things quickly. It has no widgets. If ~nice~ had been implemented on top of 
Wayland, it would have to re-implement many features, that are already part of toolkits, 
such as GTK+ or QT.

## Fluent interface

Fluent interface was abandoned during the development. C++ is simply not there yet. A lot of magic with 
templates has been tested, starting with [CRPT] (https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern), 
and ending with [our own pattern](https://stackoverflow.com/questions/62995665/c-fluent-interface). 
However, results were always too complicated and unreadable to include it into a library
which strives to simplify desktop programming.

## Friend "native" functions

All classes have some native members, which should be visible in derived classes.
An early effort to separate native code from the pure nice classes (for Microsoft Windows)
featured a lot of friend functions. After initial refactoring they were replaced by static 
member functions. Friends are just not friends when it comes to inheritance.


# Links

Following were particularly useful when developing nice.

How to handle windows state.
 * https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
 * https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992

Signals.
 * https://schneegans.github.io/tutorials/2015/09/20/signal-slot

Windows Fonts
 * http://winprog.org/tutorial/fonts.html

User defined literals
 * https://akrzemi1.wordpress.com/2012/08/12/user-defined-literals-part-i/