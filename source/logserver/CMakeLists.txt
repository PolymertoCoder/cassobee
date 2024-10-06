file(GLOB_RECURSE LOGSERVER_SRC ${SOURCE_PATH}/logserver/*.cpp)
message("logserver source: " ${LOGSERVER_SRC})

add_executable(logserver logserver.cpp ${LOGSERVER_SRC})
target_include_directories(logserver
                           PRIVATE
                           ${SOURCE_PATH}/logserver/network
)

set(LOGSERVER_DEPEND_LIB bee prot)
add_dependencies(logserver ${LOGSERVER_DEPEND_LIB})
target_link_libraries(logserver pthread ${LOGSERVER_DEPEND_LIB})

gen_startup_script(logserver)
add_common_flags(logserver)