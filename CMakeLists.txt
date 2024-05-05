cmake_minimum_required(VERSION 3.8)
project(cassobee)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE debug)

set(CMAKE_CXX_COMPILER "/usr/local/bin/c++")

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -g -std=c++17 -Wall -Wno-deprecated -Wno-unused-function -D_REENTRANT")

# include(CheckCXXCompilerFlag)
# CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
# CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
# if(COMPILER_SUPPORTS_CXX17)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#     message("The compiler ${CMAKE_CXX_COMPILER} has C++17 support.")
# elseif(COMPILER_SUPPORTS_CXX11)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#     message("The compiler ${CMAKE_CXX_COMPILER} has C++11 support.")
# else()
#     message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
# endif()

add_subdirectory(source)
