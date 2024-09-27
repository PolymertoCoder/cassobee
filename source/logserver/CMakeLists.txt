file(GLOB LOGSERVER_SRC ${SOURCE_PATH}/logserver/*.cpp)

set(LOGSERVER_DEPEND_LIB bee prot)

add_executable(logserver logserver.cpp ${LOGSERVER_SRC})
add_dependencies(logserver ${LOGSERVER_DEPEND_LIB})
target_link_libraries(logserver ${LOGSERVER_DEPEND_LIB})

gen_startup_script(logserver)
add_common_flags(logserver)