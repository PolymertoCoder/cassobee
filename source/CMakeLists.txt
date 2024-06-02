set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(.)
include_directories(./common)
include_directories(./common/logger)
include_directories(./common/utility)

aux_source_directory(./common LIBBEE_SRC)
message(${LIBBEE_SRC})
add_library(bee SHARED ${LIBBEE_SRC})

aux_source_directory(./common/logger LIBLOG_SRC)
message(${LIBLOG_SRC})
add_library(log SHARED ${LIBLOG_SRC})

set(LIB_LIB pthread bee log)

add_executable(cassobee main.cpp)
add_dependencies(cassobee bee log)
target_link_libraries(cassobee ${LIB_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug/lib)
