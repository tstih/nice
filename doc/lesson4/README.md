# Porting nice!

Hello again. If you are reading this then you've read your way through lessons
one, two, and three. It's now time to repeat everything we learned, and there's
no better way to do this than to port nice to a new platform. So in this lesson
we are going to port it to X11 together.

# Enriching CMakeLists.txt

Let's enrich our CMakeLists.txt to include X11 port and make this port default
unix port.

~~~
# We require 3.8.
cmake_minimum_required (VERSION 3.8)

# Project name is lesson 4.
project(lesson4)

# Were command line args passed?

if (WIN32 OR WIN)
    add_definitions(-DWIN32_LEAN_AND_MEAN -D__WIN__)
    add_executable (lesson4 WIN32 lesson4.cpp nice.hpp)

elseif(X11)
    add_definitions(-D__X11__)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    link_directories(${X11_LIBRARIES})
    add_definitions(${GTK3_CFLAGS_OTHER})
    add_executable (lesson4 lesson4.cpp nice.hpp)   
    target_link_libraries(lesson4 ${X11_LIBRARIES})

    # Debug version.
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_BUILD_TYPE Debug)

elseif(GTK OR UNIX)
    add_definitions(-D__GTK__)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    add_executable (lesson4 lesson4.cpp nice.hpp)
    target_link_libraries(lesson4 ${GTK3_LIBRARIES})

    # Debug version.
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_BUILD_TYPE Debug)

endif()

set_property(TARGET lesson4 PROPERTY CXX_STANDARD 20)
~~~

Because we now have three options, the `cmake` can't automatically decide what
to build. If it detects MS Windows it still builds a version for MS Windows, and
if it detects Unix it tries to build the GTK3 version. But there is no automation
for X11. You have to pass this request as argument to cmake `cmake -DX11:1`.

 > Sinilarly you can implicitly require to build versions for GTK (`cmake -DGTK:1`) 
 > and MS Windows (`cmake -DWIN:1`). 

# How to start?

Starting the port is failry easy. You need to examine all the places with platform
dependant code. You can find those places by searching for `__WIN__` or `__GTK__`
processor directives.

## Includes

## Unified types

## Extending the resource class

## Application
### Application instance
### Primary instance check
### Application entry point
### The message pump

## Window
### Events
### Create
### Destroy

## Artist




