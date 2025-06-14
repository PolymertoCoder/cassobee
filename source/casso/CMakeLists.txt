# cassobee exe

file(GLOB_RECURSE CASSOBEE_SRC ${SOURCE_PATH}/casso/*.cpp)

add_executable(cassobee ${CASSOBEE_SRC}
                        ${SOURCE_PATH}/protocol/state/state_clientserver.cpp
)

target_include_directories(cassobee
                           PRIVATE
                           ${SOURCE_PATH}/casso/network
)

target_compile_definitions(cassobee PRIVATE -D_REENTRANT)

target_link_libraries(cassobee PUBLIC pthread bee_r logclient_r prot_r)

add_common_flags(cassobee)

gen_startup_script(cassobee)