set(CMAKE_VERBOSE_MAKEFILE ON)

function(add_common_flags target)
    get_target_property(sources ${target} SOURCES)

    foreach(source ${sources})
        get_filename_component(filename ${source} NAME)
        set_source_files_properties(${source} PROPERTIES COMPILE_DEFINITIONS "__FILENAME__=\"${filename}\"")
    endforeach()
endfunction()

function(gen_startup_script target)
    add_custom_command(
        OUTPUT ${BINARY_PATH}/run_${target}.sh ${BINARY_PATH}/gdbrun_${target}.sh
        COMMAND ${CMAKE_COMMAND} -E env python3 ${TOOLS_PATH}/build_tool/gen_startup_script.py ${EXECUTABLE_OUTPUT_PATH} ${BINARY_PATH} ${target} ${CONFIG_PATH} ${DEBUG_PATH}
        DEPENDS ${target} ${TOOLS_PATH}/build_tool/gen_startup_script.py
        #COMMENT Generating run_${target}.sh and gdbrun_${target}.sh
    )

    add_custom_target(
        gen_startup_script_${target} ALL
        DEPENDS ${BINARY_PATH}/run_${target}.sh ${BINARY_PATH}/gdbrun_${target}.sh
        #COMMENT Generating all startup scripts for ${target}
    )

    #add_dependencies(${target} gen_startup_script_${target})
endfunction()

function(add_sanitize_flag target)
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_libraries(${target} -fsanitize=address)
endfunction()

include_directories(.
                    ./common
                    ./common/io
                    ./common/logger
                    ./common/traits
                    ./common/utility
                    ./common/security
                    ./protocol
                    ./protocol/include
)

# libbee
aux_source_directory(./common LIBBEE_SRC)
aux_source_directory(./common/io LIBBEE_SRC)
aux_source_directory(./common/traits LIBBEE_SRC)
aux_source_directory(./common/utility LIBBEE_SRC)
message("libbee source: " ${LIBBEE_SRC})
add_library(bee SHARED ${LIBBEE_SRC})

# liblog
aux_source_directory(./common/logger LIBLOG_SRC)
message("liblog source: " ${LIBLOG_SRC})
add_library(log SHARED ${LIBLOG_SRC})

# libprot
aux_source_directory(./protocol/source LIBPROT_SRC)
include_directories(./protocol/include)
add_library(prot SHARED ${LIBPROT_SRC})

# libsecurity
aux_source_directory(./common/security LIBSECURITY_SRC)
message("libsecurity source: " ${LIBSECURITY_SRC})
add_library(security SHARED ${LIBSECURITY_SRC})
find_library(LIBCRYTOPP_PATH cryptopp ${THIRDPARTY_PATH}/cryptopp)
target_link_libraries(security ${LIBCRYTOPP_PATH})

# cassobee exe
set(CASSOBEE_DEPEND_LIB bee log security prot)

add_executable(cassobee main.cpp)
add_dependencies(cassobee ${CASSOBEE_DEPEND_LIB})
target_link_libraries(cassobee pthread ${CASSOBEE_DEPEND_LIB})

# client exe
add_subdirectory(client)

add_common_flags(bee)
add_common_flags(log)
add_common_flags(cassobee)

gen_startup_script(cassobee)

if(${CASSOBEE_SENITIZE_ENABLE})
    add_sanitize_flag(bee)
    add_sanitize_flag(log)
    add_sanitize_flag(cassobee)
endif()

if(${CASSOBEE_BENCHMARK_ENABLE})
    set(BENCHMARK_ENABLE_TESTING OFF)
    add_subdirectory(${THIRDPARTY_PATH}/benchmark ${CMAKE_BINARY_DIR}/thirdparty/benchmark)
    add_subdirectory(benchmark)
endif()
