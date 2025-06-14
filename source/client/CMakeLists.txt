# client exe

file(GLOB_RECURSE CLIENT_SRC ${SOURCE_PATH}/client/*.cpp)

add_executable(client ${CLIENT_SRC}
                      ${SOURCE_PATH}/protocol/state/state_clientserver.cpp
)

target_include_directories(client
                           PRIVATE
                           ${SOURCE_PATH}/client/network
)

target_compile_definitions(client PRIVATE -D_REENTRANT)

target_link_libraries(client PUBLIC pthread bee_r logclient_r prot_r)

add_common_flags(client)

gen_startup_script(client)