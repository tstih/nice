# We require 3.8.
cmake_minimum_required (VERSION 3.8)

# Project name is lesson 6.
project(lesson6)

# Were command line args passed?

if (WIN32 OR WIN)
    add_definitions(-DWIN32_LEAN_AND_MEAN -D__WIN__)
    add_executable (lesson6 WIN32 lesson6.cpp nice.hpp)

elseif(X11)
    add_definitions(-D__X11__)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    link_directories(${X11_LIBRARIES})
    add_executable (lesson6 lesson6.cpp nice.hpp)   
    target_link_libraries(lesson6 ${X11_LIBRARIES})

    # Debug version.
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_BUILD_TYPE Debug)

elseif(MOTIF)
    add_definitions(-D__MOTIF__)

    find_package(X11 REQUIRED)
    find_package(Motif REQUIRED)
    
    link_directories(${X11_LIBRARIES})
    
    include_directories(${X11_INCLUDE_DIR} ${MOTIF_INCLUDE_DIR})
    
    add_executable (lesson6 lesson6.cpp nice.hpp) 
    
    target_link_libraries(lesson6 ${X11_LIBRARIES} ${MOTIF_LIBRARIES} Xt X11)

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
    add_executable (lesson6 lesson6.cpp nice.hpp)
    target_link_libraries(lesson6 ${GTK3_LIBRARIES})

    # Debug version.
    set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
    set(CMAKE_BUILD_TYPE Debug)

endif()

set_property(TARGET lesson6 PROPERTY CXX_STANDARD 20)
