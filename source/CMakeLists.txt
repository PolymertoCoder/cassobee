set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(.
                    ./common
                    ./common/io
                    ./common/logger
                    ./common/utility
)

aux_source_directory(./common LIBBEE_SRC)
aux_source_directory(./common/io LIBBEE_SRC)
aux_source_directory(./common/utility LIBBEE_SRC)
message("libbee source: " ${LIBBEE_SRC})
add_library(bee SHARED ${LIBBEE_SRC})

aux_source_directory(./common/logger LIBLOG_SRC)
message("liblog source: " ${LIBLOG_SRC})
add_library(log SHARED ${LIBLOG_SRC})

set(LIB_LIB pthread bee log)

add_executable(cassobee main.cpp)
add_dependencies(cassobee bee log)
target_link_libraries(cassobee ${LIB_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug/lib)
