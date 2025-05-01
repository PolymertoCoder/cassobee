# cassobee exe

file(GLOB_RECURSE CASSOBEE_SRC ${SOURCE_PATH}/casso/*.cpp)

add_executable(cassobee ${CASSOBEE_SRC})

target_include_directories(cassobee
                           PRIVATE
                           ${SOURCE_PATH}/casso/network
)

target_compile_definitions(cassobee PRIVATE -D_REENTRANT)

target_link_libraries(cassobee pthread bee_r logclient_r prot)

add_common_flags(cassobee)

gen_startup_script(cassobee)