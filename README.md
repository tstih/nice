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

size
rct

## Fluent interface

The problem of before window creation and after window creation functions...