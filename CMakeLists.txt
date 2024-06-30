cmake_minimum_required(VERSION 3.8)
project(cassobee)

set(CASSOBEE_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(THIRDPARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CASSOBEE_BENCHMARK_ENABLE ON)
set(CASSOBEE_IWYU_ENABLE ON)

set(CMAKE_BUILD_TYPE debug)

#set(CMAKE_C_COMPILER "/usr/local/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++")
set(CMAKE_C_COMPILER "/usr/local/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")

if(${CASSOBEE_IWYU_ENABLE})
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "usr/local/bin/include-what-you-use")
endif()

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -g -std=c++20 -Wall -Wno-deprecated -Wno-unused-function -D_REENTRANT")

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

add_compile_options(-DCMAKE_CROSSCOMPILING=true)

if(${CASSOBEE_BENCHMARK_ENABLE})
    #target_link_library()
endif()

add_subdirectory(source)
