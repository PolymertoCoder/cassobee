cmake_minimum_required(VERSION 3.8)
project(cassobee)

message("Use ${CMAKE_CXX_COMPILER_ID} compiler to build this project.")

set(CASSOBEE_ROOT_PATH      ${CMAKE_CURRENT_SOURCE_DIR})
set(CONFIG_PATH             ${CASSOBEE_ROOT_PATH}/config)
set(SOURCE_PATH             ${CASSOBEE_ROOT_PATH}/source)
set(THIRDPARTY_PATH         ${CASSOBEE_ROOT_PATH}/thirdparty)
set(EXECUTABLE_OUTPUT_PATH  ${CASSOBEE_ROOT_PATH}/debug)
set(DEBUG_PATH              ${CASSOBEE_ROOT_PATH}/debug)
set(THIRDLIB_OUTPUT_PATH    ${CASSOBEE_ROOT_PATH}/debug)
set(LIBRARY_OUTPUT_PATH     ${CASSOBEE_ROOT_PATH}/debug/libs)
set(BINARY_PATH             ${CASSOBEE_ROOT_PATH}/bin)
set(TOOLS_PATH              ${CASSOBEE_ROOT_PATH}/tools)
set(PROTOCOL_PATH           ${CASSOBEE_ROOT_PATH}/source/protocol)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(CASSOBEE_BENCHMARK_ENABLE "enable benchmark test" OFF)
option(CASSOBEE_IWYU_ENABLE "enable include-what-you-use" OFF)
option(CASSOBEE_SENITIZE_ENABLE "enable google sanitizer" ON)
option(CASSOBEE_CCACHE_ENABLE "enable ccache" ON)
option(CASSOBEE_CLANG_TIDY_ENABLE "enable clang-tidy" OFF)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

#set(CMAKE_C_COMPILER "/usr/local/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/g++")
#set(CMAKE_C_COMPILER "/usr/local/bin/clang")
#set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -std=c++23 -Wall -Wno-deprecated -Wno-unused-function -Wno-interference-size -D_REENTRANT")

# debug or release
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
   add_compile_options(-O0 -ggdb -g)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
   add_compile_options(-O3)
endif()

add_compile_options(-DCMAKE_CROSSCOMPILING=true)

# enable include-what-you-use
if(${CASSOBEE_IWYU_ENABLE})
    find_program(IWYU_BIN_PATH "include-what-you-use")
    if(IWYU_BIN_PATH)
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE '${IWYU_BIN_PATH};-Xiwyu;--transitive_includes_only;')
    else()
        message(FATAL_ERROR "Cound not found include-what-you-use.")
    endif()
endif()

# enable ccache
if(${CASSOBEE_CCACHE_ENABLE})
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message("Find ccache path: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    else()
        message(FATAL_ERROR "Could not find ccache.")
    endif()
endif()

# enable clang-tidy
# Clang-tidy custom targets for running and checking results
add_custom_target(run_clang_tidy
    COMMAND clang-tidy -p ${CMAKE_BINARY_DIR} --warnings-as-errors=* ${ALL_CPP_FILES} || ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/clang_tidy_failed
    COMMENT "Running clang-tidy"
    VERBATIM
)

add_custom_target(check_clang_tidy
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/clang_tidy_failed
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target run_clang_tidy
    COMMENT "Checking clang-tidy results"
    VERBATIM
)

# Function to add Clang-Tidy pre-build check
function(add_clang_tidy_pre_build target)
    add_custom_command(TARGET ${target} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running Clang-Tidy pre-build check for ${target}"
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/clang_tidy_failed
        COMMAND /bin/bash -c "if [ -f ${CMAKE_BINARY_DIR}/clang_tidy_failed ]; then echo 'Stopping build due to Clang-tidy errors.'; exit 1; else echo 'No Clang-tidy issues found. Continuing build.'; fi"
        COMMENT "Checking for Clang-tidy issues before building ${target}"
        VERBATIM
    )
endfunction()

if(${CASSOBEE_CLANG_TIDY_ENABLE})
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message("Find clang-tidy exe: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
    else()
        message(FATAL_ERROR "Could not find the clang-tidy exe")
    endif()
endif()

# generate protocol
add_custom_target(
    generate_protocol ALL
    COMMAND pg
    DEPENDS ${CASSOBEE_ROOT_PATH}/progen/*.xml
    COMMENT "Generating protocol..."
)

# # build thirdparty libraries
# set(THIRDPARTY_LIB_PATHS 
#     ${THIRDLIB_OUTPUT_PATH}/libs/libcryptopp.a
#     ${THIRDLIB_OUTPUT_PATH}/libs/libreadline.a
#     ${THIRDLIB_OUTPUT_PATH}/libs/libhistory.a
#     ${THIRDLIB_OUTPUT_PATH}/libs/libtinyxml2.so
#     ${THIRDLIB_OUTPUT_PATH}/mysql-connector-cpp/libconnector.so
# )

# add_custom_command(
#     OUTPUT ${THIRDPARTY_LIB_PATHS}
#     COMMAND build_thirdparty
#     COMMENT "Building thirdparty libraries..."
# )

# add_custom_target(build_thirdparty_libraries ALL DEPENDS ${THIRDPARTY_LIB_PATHS})

# libtinyxml
set(tinyxml2_BUILD_TESTING OFF)
set(tinyxml2_SHARED_LIBS ${LIBRARY_OUTPUT_PATH})
add_subdirectory(${THIRDPARTY_PATH}/tinyxml2)

# readline
find_library(LIBREADLINE_PATH
             NAMES readline
             PATHS ${LIBRARY_OUTPUT_PATH}
             NO_DEFAULT_PATH
)
if(LIBREADLINE_PATH)
    message(STATUS "Found readline library: ${LIBREADLINE_PATH}")
else()
    message(FATAL_ERROR "Could not find the readline library")
endif()

# crypto++
find_library(LIBCRYPTOPP_PATH
             NAMES cryptopp
             PATHS ${LIBRARY_OUTPUT_PATH}
             NO_DEFAULT_PATH
)
if(LIBCRYPTOPP_PATH)
    message(STATUS "Found cryptopp library: ${LIBCRYPTOPP_PATH}")
else()
    message(FATAL_ERROR "Could not find the cryptopp library")
endif()

# mysqlcppconn
find_library(LIBMYSQLCPPCONN_PATH
             NAMES mysqlcppconn
             PATHS ${LIBRARY_OUTPUT_PATH}
             NO_DEFAULT_PATH
)
if(LIBMYSQLCPPCONN_PATH)
    message(STATUS "Found mysqlcppconn library: ${LIBMYSQLCPPCONN_PATH}")
else()
    message(FATAL_ERROR "Could not find the mysqlcppconn library")
endif()

add_subdirectory(source)
