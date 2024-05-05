set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(.)
include_directories(./common)

aux_source_directory(./common LIB_SRC)
message(${LIB_SRC})

add_library(cassobee SHARED ${LIB_SRC})

set(LIB_LIB pthread cassobee)

add_executable(main main.cpp)
add_dependencies(main cassobee)
target_link_libraries(main ${LIB_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug/lib)
