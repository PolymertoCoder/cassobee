cmake_minimum_required(VERSION 3.8)

#set(CMAKE_C_COMPILER "/usr/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(cassobee LANGUAGES CXX)

message("Use ${CMAKE_CXX_COMPILER_ID} compiler to build this project.")

set(CASSOBEE_ROOT_PATH      ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_PATH              ${CMAKE_CURRENT_SOURCE_DIR}/build)
set(CONFIG_PATH             ${CASSOBEE_ROOT_PATH}/config)
set(SOURCE_PATH             ${CASSOBEE_ROOT_PATH}/source)
set(THIRDPARTY_PATH         ${CASSOBEE_ROOT_PATH}/thirdparty)
set(EXECUTABLE_OUTPUT_PATH  ${CASSOBEE_ROOT_PATH}/debug)
set(DEBUG_PATH              ${CASSOBEE_ROOT_PATH}/debug)
set(THIRDLIB_OUTPUT_PATH    ${CASSOBEE_ROOT_PATH}/debug)
set(LIBRARY_OUTPUT_PATH     ${CASSOBEE_ROOT_PATH}/debug/libs)
set(REPORT_PATH             ${CASSOBEE_ROOT_PATH}/debug)
set(BINARY_PATH             ${CASSOBEE_ROOT_PATH}/bin)
set(TOOLS_PATH              ${CASSOBEE_ROOT_PATH}/tools)
set(PROTOCOL_PATH           ${CASSOBEE_ROOT_PATH}/source/protocol)
set(CMAKE_PATH              ${CASSOBEE_ROOT_PATH}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(CASSOBEE_BENCHMARK_ENABLE "enable benchmark test" OFF)
option(CASSOBEE_IWYU_ENABLE "enable include-what-you-use" OFF)
option(CASSOBEE_SENITIZE_ENABLE "enable google sanitizer" ON)
option(CASSOBEE_CCACHE_ENABLE "enable ccache" ON)
option(CASSOBEE_CLANG_TIDY_ENABLE "enable clang-tidy" OFF)
option(CASSOBEE_COMPILE_TIME_REPORT_ENABLE "enable compile time report" OFF)
option(CASSOBEE_JEMALLOC_ENABLE "enable jemalloc allocator" ON)
option(CASSOBEE_REFLECTION_ENABLE "enable refletion generation" OFF)
option(CASSOBEE_GEN_FASTBUILD_BFF_ENABLE "enable generate fastbuild bff" OFF)

set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)

# 自定义cmake模块目录
list(APPEND CMAKE_MODULE_PATH ${CMAKE_PATH})

# 设置通用的编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{CXXFLAGS} -std=c++23 -Wall -Wno-deprecated -Wno-unused-function")

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
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_BIN_PATH}"
                                            "-Xiwyu" "--verbose=3"  # 详细输出
                                            "-Xiwyu" "--no_comments"  # 移除注释
                                            "-Xiwyu" "--transitive_includes_only"
                                            "-Xiwyu" "--error"  # 将警告提升为错误
                                            "-Xiwyu" "--output=${REPORT_PATH}/iwyu_report.json"  # 输出文件名
                                            "-Xiwyu" "--format=json"  # JSON 格式
        )
        set(CMAKE_CXX_CLANG_TIDY_OUTPUT_DIRECTORY ${IWYU_REPORT_DIR}) # 设置全局输出重定向
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

# enable compile time report
if(${CASSOBEE_COMPILE_TIME_REPORT_ENABLE})
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-ftime-trace)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/Bt)
    else()
        message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not support compile time trace.")
    endif()
endif()

# enable clang-tidy
# Clang-tidy custom targets for running and checking results
add_custom_target(run_clang_tidy
    COMMAND clang-tidy -p ${BUILD_PATH} --warnings-as-errors=* || ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/clang_tidy_failed
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
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE}
                                 "--format=sarif"  # SARIF 格式
                                 "--export-fixes=${REPORT_PATH}/clang-tidy-fixes.yaml" # 自动修复建议
                                 "-p=${BUILD_PATH}" # 指定编译数据库路径
        )
        set(CMAKE_CXX_CLANG_TIDY_OUTPUT_DIRECTORY ${REPORT_PATH})
    else()
        message(FATAL_ERROR "Could not find the clang-tidy exe")
    endif()
endif()

if(${CASSOBEE_JEMALLOC_ENABLE})
    find_library(LIBJEMALLOC
                 NAMES jemalloc
                 PATHS ${LIBRARY_OUTPUT_PATH}
                 NO_DEFAULT_PATH
    )
    if(LIBJEMALLOC)
        message(STATUS "Found jemalloc library: ${LIBJEMALLOC}")
    else()
        message(FATAL_ERROR "Could not find the jemalloc library")
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

# cmake_language(DEFER CALL _execute_after_all_targets ${CMAKE_PATH})

# function(_execute_after_all_targets module_path)
#     message(STATUS "Executing deferred FastBuild generation")
#     list(APPEND CMAKE_MODULE_PATH ${module_path})
#     include(fastbuild_gen)
#     generate_fastbuild_config()
# endfunction()

# 根据cmake规则生成fastbuild bff文件，必须放在最后
if(${CASSOBEE_GEN_FASTBUILD_BFF_ENABLE})
    message(STATUS "=== FASTBUILD GENERATION START ===")
    
    # 打印当前定义的目标
    get_property(TARGET_NAMES DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    message(STATUS "Detected targets: ${TARGET_NAMES}")
    
    # 包含模块
    include(fastbuild_gen)
    
    # 生成配置
    generate_fastbuild_config()
    
    message(STATUS "=== FASTBUILD GENERATION COMPLETE ===")
endif()