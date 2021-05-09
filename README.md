![status.badge] [![language.badge]][language.url] [![standard.badge]][standard.url] [![license.badge]][license.url]
 
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
 * multiplatform.

# Hello nice

Here's the Hello World application in nice:

~~~cpp
#include "nice.hpp"

void program()
{
    nice::app::run(app_wnd("Hello World!"));
}
~~~

And here's the Hello Paint application in nice:

~~~cpp
#include "nice.hpp"

class main_wnd : public nice::app_wnd {
public:
    main_wnd() : app_wnd("Hello paint!") {
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    // Paint handler, draws rectangle.
    void on_paint(const nice::artist& a) {
        a.draw_rect({ 255,0,0 }, { 10,10,100,100 });
    }
};

void program()
{
    nice::app::run(main_wnd());
}
~~~

# Compiling

Open your terminal application and clone the git archive with

`git clone --recursive https://github.com/tstih/nice.git`

The `--recursive` flag will get the repository and all submodules
i.e. dependencies. Then `cd` to the repository folder and execute
build command (see following two chapters for the correct build 
command).

## Windows

Make sure your `vcvarsall.bat` (Visual Studio variables) are all set.

The build command is

~~~
nmake /F Makefile.nmake
~~~

## Linux

The build command is

~~~
make x11
~~~

Other options for Linux (at time of writing) are: `gtk`.

# Status

## Done
 * proof of concept: hello world
 * transformed into single header library
 * ms windows binding
 * refactoring no 1
 * GTK3 binding
 * mapping window messages to C++ signals
 * basic paint proof of concept
 * refactoring no 2
 * X11 binding
 * scribble app

## Implementing
 * basic layout manager POC
 * calculator app
 * refactoring no 3

## Planning
 * custom controls
 * improve painting, optimize performance
 * refactoring no 4
 * multithreading operations


# Nice tutorial

Part 1: [Nice application.](https://github.com/tstih/nice/tree/master/doc/lesson1)

Part 2: [Basic windows.](https://github.com/tstih/nice/tree/master/doc/lesson2)

Part 3: [Window message routing, and painting.](https://github.com/tstih/nice/tree/master/doc/lesson3)

Part 4: [Porting nice to a new platform: X11](https://github.com/tstih/nice/tree/master/doc/lesson4)

Part 5: [The Scrible!](https://github.com/tstih/nice/tree/master/doc/lesson5)

Part 6: Child windows and layout managers (unfinished)

Part 7: Custom controls (unfinished)

Part 8: Adding a handful of common controls (unfinished)

Part 9: The style (unfinished)

Part 10: At the end it's all about being nice (unfinished)


# Bumps on the road

The mistakes we made and lessons we took on the way.

## To Wayland or not to Wayland?

Perhaps one day we will Wayland, but at this moment we won't. Wayland is a technology for drawing things quickly. It has no widgets. If ~nice~ had been implemented on top of Wayland, it would have to re-implement many features that are already part of toolkits, such as GTK+ or QT.

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

PIMPL for GUI
 * https://stackoverflow.com/questions/35191894/pimpl-with-inheritance-using-smart-pointer


[language.url]:   https://isocpp.org/
[language.badge]: https://img.shields.io/badge/language-C++-blue.svg

[standard.url]:   https://en.wikipedia.org/wiki/C%2B%2B#Standardization
[standard.badge]: https://img.shields.io/badge/C%2B%2B-20-blue.svg

[license.url]:    https://github.com/tstih/nice/blob/master/LICENSE
[license.badge]:  https://img.shields.io/badge/license-MIT-blue.svg

[status.badge]:  https://img.shields.io/badge/status-unstable-red.svg