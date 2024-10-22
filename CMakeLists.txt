cmake_minimum_required(VERSION 3.8)
project(cassobee)

message("Use ${CMAKE_CXX_COMPILER_ID} compiler to build this project.")

set(CASSOBEE_ROOT_PATH      ${CMAKE_CURRENT_SOURCE_DIR})
set(CONFIG_PATH             ${CASSOBEE_ROOT_PATH}/config)
set(SOURCE_PATH             ${CASSOBEE_ROOT_PATH}/source)
set(THIRDPARTY_PATH         ${CASSOBEE_ROOT_PATH}/thirdparty)
set(EXECUTABLE_OUTPUT_PATH  ${CASSOBEE_ROOT_PATH}/debug)
set(DEBUG_PATH              ${CASSOBEE_ROOT_PATH}/debug)
set(LIBRARY_OUTPUT_PATH     ${CASSOBEE_ROOT_PATH}/debug/libs)
set(BINARY_PATH             ${CASSOBEE_ROOT_PATH}/bin)
set(TOOLS_PATH              ${CASSOBEE_ROOT_PATH}/tools)
set(PROTOCOL_PATH           ${CASSOBEE_ROOT_PATH}/source/protocol)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(CASSOBEE_BENCHMARK_ENABLE "enable benchmark test" OFF)
option(CASSOBEE_IWYU_ENABLE "enable include-what-you-use" OFF)
option(CASSOBEE_SENITIZE_ENABLE "enable google senitizer" ON)
option(CASSOBEE_CCACHE_ENABLE "enable ccache" ON)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

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

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -std=c++23 -Wall -Wno-deprecated -Wno-unused-function -Wno-interference-size -D_REENTRANT")

if(${CMAKE_BUILD_TYPE} EQUAL Debug)
   add_compile_options(-O0 -ggdb -g)
elif(${CMAKE_BUILD_TYPE} EQUAL Release)
   add_compile_options(-O3)
endif()

add_compile_options(-DCMAKE_CROSSCOMPILING=true)

if(${CASSOBEE_CCACHE_ENABLE})
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message("Find ccache path: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    endif()
endif()

add_custom_target(
    generate_protocol ALL
    COMMAND pg
    DEPENDS ${CASSOBEE_ROOT_PATH}/progen/*.xml
    COMMENT "Generating protocol..."
)

add_subdirectory(source)

set(tinyxml2_BUILD_TESTING OFF)
set(tinyxml2_SHARED_LIBS ${LIBRARY_OUTPUT_PATH})
add_subdirectory(${THIRDPARTY_PATH}/tinyxml2)
