set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_policy(SET CMP0079 NEW)

function(add_common_flags target)
    get_target_property(sources ${target} SOURCES)

    foreach(source ${sources})
        get_filename_component(filename ${source} NAME)
        set_source_files_properties(${source} PROPERTIES COMPILE_DEFINITIONS "__FILENAME__=\"${filename}\"")
    endforeach()

    if(${CASSOBEE_SENITIZE_ENABLE})
        add_sanitize_flag(${target})
    endif()

    if(${CASSOBEE_CLANG_TIDY_ENABLE})
        # Ensure run_clang_tidy and check_clang_tidy are run before building any target
        add_dependencies(${target} run_clang_tidy)
        add_clang_tidy_pre_build(${target})
    endif()

    if(${CASSOBEE_JEMALLOC_ENABLE})
        target_link_libraries(${target} ${LIBJEMALLOC})
    endif()
endfunction()

function(gen_startup_script target)
    add_custom_command(
        OUTPUT ${BINARY_PATH}/run_${target}.sh ${BINARY_PATH}/gdbrun_${target}.sh
        COMMAND ${CMAKE_COMMAND} -E env python3 ${TOOLS_PATH}/build_tool/gen_startup_script.py ${EXECUTABLE_OUTPUT_PATH} ${BINARY_PATH} ${target} ${CONFIG_PATH} ${DEBUG_PATH}
        DEPENDS ${target} ${TOOLS_PATH}/build_tool/gen_startup_script.py
        #COMMENT "Generating run_${target}.sh and gdbrun_${target}.sh"
    )

    add_custom_target(
        gen_startup_script_${target} ALL
        DEPENDS ${BINARY_PATH}/run_${target}.sh ${BINARY_PATH}/gdbrun_${target}.sh
        #COMMENT "Generating all startup scripts for ${target}"
    )
endfunction()

function(add_sanitize_flag target)
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_libraries(${target} -fsanitize=address)
endfunction()

# libprot
file(GLOB LIBPROT_SRC ${SOURCE_PATH}/protocol/source/*.cpp)
add_library(prot SHARED ${LIBPROT_SRC})
set(LIBPROT_DEPEND_LIB bee)
add_dependencies(prot ${LIBPROT_DEPEND_LIB})
add_dependencies(prot generate_protocol)
target_link_libraries(prot ${LIBPROT_DEPEND_LIB})
target_include_directories(prot PUBLIC ./protocol/include)

# libbee
add_subdirectory(bee)

# liblogclient
add_subdirectory(logclient)

# cassobee exe
add_subdirectory(casso)

# client exe
add_subdirectory(client)

# logserver exe
add_subdirectory(logserver)

if(${CASSOBEE_BENCHMARK_ENABLE})
    set(BENCHMARK_ENABLE_TESTING OFF)
    add_subdirectory(${THIRDPARTY_PATH}/benchmark ${CMAKE_BINARY_DIR}/thirdparty/benchmark)
    add_subdirectory(benchmark)
endif()
