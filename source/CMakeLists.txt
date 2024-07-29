set(CMAKE_VERBOSE_MAKEFILE ON)

function(add_common_flags target)
    get_target_property(sources ${target} SOURCES)

    foreach(source ${sources})
        get_filename_component(filename ${source} NAME)
        set_source_files_properties(${source} PROPERTIES COMPILE_DEFINITIONS "__FILENAME__=\"${filename}\"")
    endforeach()
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

# libsecurity
aux_source_directory(./common/security LIBSECURITY_SRC)
message("libsecurity source: " ${LIBSECURITY_SRC})
add_library(security SHARED ${LIBSECURITY_SRC})
find_library(LIBCRYTOPP_PATH cryptopp ${THIRDPARTY_PATH}/cryptopp)
target_link_libraries(security ${LIBCRYTOPP_PATH})

set(LIB_LIB pthread bee log security)

add_executable(cassobee main.cpp)
add_dependencies(cassobee bee log)
target_link_libraries(cassobee ${LIB_LIB})

add_common_flags(bee)
add_common_flags(log)
add_common_flags(cassobee)

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
