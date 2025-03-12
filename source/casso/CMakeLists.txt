# cassobee exe

file(GLOB_RECURSE CASSOBEE_SRC ${SOURCE_PATH}/casso/*.cpp)
file(GLOB_RECURSE CASSOBEE_SRC ${SOURCE_PATH}/logclient/*.cpp)

add_executable(cassobee ${CASSOBEE_SRC} ${LOGCLIENT_SRC})

target_include_directories(cassobee
                           PRIVATE
                           ${SOURCE_PATH}/casso/network
                           ${SOURCE_PATH}/logclient
)

target_link_libraries(cassobee pthread bee prot)

add_common_flags(cassobee)

gen_startup_script(cassobee)