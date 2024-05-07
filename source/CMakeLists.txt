set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(.)
include_directories(./common)

aux_source_directory(./common LIB_SRC)
message(${LIB_SRC})

add_library(bee SHARED ${LIB_SRC})

set(LIB_LIB pthread bee)

add_executable(cassobee main.cpp)
add_dependencies(cassobee bee)
target_link_libraries(cassobee ${LIB_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug/lib)
