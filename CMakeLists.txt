cmake_minimum_required(VERSION 3.8)
project(cassobee)

message("use ${CMAKE_CXX_COMPILER_ID} compiler to build this project.")

set(CASSOBEE_ROOT_PATH  ${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCE_PATH         ${CASSOBEE_ROOT_PATH}/source)
set(THIRDPARTY_PATH     ${CASSOBEE_ROOT_PATH}/thirdparty)

option(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(CASSOBEE_BENCHMARK_ENABLE ON)
option(CASSOBEE_IWYU_ENABLE OFF)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

#set(CMAKE_C_COMPILER "/usr/local/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++")
#set(CMAKE_C_COMPILER "/usr/local/bin/clang")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")

if(${CASSOBEE_IWYU_ENABLE})
    find_program(IWYU_BIN_PATH "include-what-you-use")
    if(IWYU_BIN_PATH)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE '${IWYU_BIN_PATH};-Xiwyu;--transitive_includes_only;')
    else()
        message(FATAL_ERROR "Cound not found include-what-you-use.")
    endif()
endif()

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -std=c++20 -Wall -Wno-deprecated -Wno-unused-function -D_REENTRANT")

if(${CMAKE_BUILD_TYPE} EQUAL debug)
   add_compile_options(-O0 -ggdb -g)
elif(${CMAKE_BUILD_TYPE} EQUAL release)
   add_compile_options(-O2)
endif()

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

add_subdirectory(source)
