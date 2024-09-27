# client exe

set(CLIENT_DEPEND_LIB bee security prot)

add_executable(client client.cpp)
add_dependencies(client ${CLIENT_DEPEND_LIB})
target_link_libraries(client pthread ${CLIENT_DEPEND_LIB})

add_common_flags(client)
gen_startup_script(client)