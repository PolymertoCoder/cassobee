# cassobee exe
set(CASSOBEE_DEPEND_LIB bee log security prot)

file(GLOB_RECURSE CASSOBEE_SRC ${SOURCE_PATH}/casso/*.cpp)

add_executable(cassobee main.cpp)

target_include_directories(cassobee
                           PRIVATE
                           ${SOURCE_PATH}/casso/network
)

add_dependencies(cassobee ${CASSOBEE_DEPEND_LIB})
target_link_libraries(cassobee pthread ${CASSOBEE_DEPEND_LIB})

add_common_flags(cassobee)
gen_startup_script(cassobee)