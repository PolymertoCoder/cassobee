set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Wno-unused-function")

include_directories(.)
include_directories(./reactor)
include_directories(./threadpool)
include_directories(./delegate)

set(LIB_SRC util.cpp)
add_library(cassobee SHARED ${LIB_SRC})

set(LIB_LIB pthread cassobee)

add_executable(main main.cpp)
add_dependencies(main cassobee)
target_link_libraries(main ${LIB_LIB})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/debug/lib)
