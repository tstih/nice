# Welcome to nice
Nice is an experimental modern C++ library for building graphical user interfaces. 

Hello world in nice:
~~~
#include "nice.hpp"

void program()
{
    nice::app::run(std::make_shared<nice::app_wnd>(L"Hello world!"));
}
~~~

# Status

## Done
 * proof of concept: hello world
 * transformed into single header library
 * ms windows binding
 * mapping window messages to C++ signals

## Implementing
 * wayland binding

## Planning
 * exceptions
 * fluent interface
 * abstracting drawing primitives
 * standard controls (buttons, scrollbars, text edit)