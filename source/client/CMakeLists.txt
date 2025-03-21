# client exe

file(GLOB_RECURSE CLIENT_SRC ${SOURCE_PATH}/client/*.cpp)

add_executable(client ${CLIENT_SRC})

target_include_directories(client
                           PRIVATE
                           ${SOURCE_PATH}/client/network
)

target_compile_definitions(client PRIVATE -D_REENTRANT)

target_link_libraries(client pthread bee_r logclient)

add_common_flags(client)

gen_startup_script(client)