file(GLOB_RECURSE LOGSERVER_SRC ${SOURCE_PATH}/logserver/*.cpp)

add_executable(logserver ${LOGSERVER_SRC})

target_include_directories(logserver PRIVATE ${SOURCE_PATH}/logserver/network)

target_link_libraries(logserver pthread bee_r prot logclient)

gen_startup_script(logserver)

add_common_flags(logserver)