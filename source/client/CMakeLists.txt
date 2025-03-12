# client exe

file(GLOB_RECURSE CLIENT_SRC ${SOURCE_PATH}/client/*.cpp)
file(GLOB_RECURSE CLIENT_SRC ${SOURCE_PATH}/logclient/*.cpp)

add_executable(client ${CLIENT_SRC} ${LOGCLIENT_SRC})

target_include_directories(client
                           PRIVATE
                           ${SOURCE_PATH}/client/network
                           ${SOURCE_PATH}/client
                           ${SOURCE_PATH}/logclient
)
target_link_libraries(client pthread bee)

add_common_flags(client)
gen_startup_script(client)