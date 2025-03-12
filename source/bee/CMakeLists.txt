# log module


# security module
set(SECURITY_LINK_LIB ${LIBCRYPTOPP_PATH})
set(SECURITY_INCLUDE ${THIRDPARTY_PATH}/cryptopp)

# libcli
set(CLI_LINK_LIB ncurses)
set(CLI_INCLUDE ${THIRDPARTY_PATH}/readline
                ${THIRDLIB_OUTPUT_PATH}/readline/include
)

# libcmysql
add_subdirectory(database)

# libbee
file(GLOB_RECURSE LIBBEE_SRC ${SOURCE_PATH}/bee/*.cpp)
message("libbee source: " ${LIBBEE_SRC})
add_library(bee SHARED ${LIBBEE_SRC})
target_include_directories(bee
                           PUBLIC
                           ${SOURCE_PATH}/bee/common
                           ${SOURCE_PATH}/bee/io
                           ${SOURCE_PATH}/bee/log
                           ${SOURCE_PATH}/bee/traits
                           ${SOURCE_PATH}/bee/utility
                           ${SOURCE_PATH}/bee/thread
                           ${SOURCE_PATH}/protocol
                           ${SOURCE_PATH}/protocol/include
                           PRIVATE
                           ${CLI_INCLUDE}
                           ${SECURITY_INCLUDE}
)

target_link_libraries(bee
                      atomic
                      ${CLI_LINK_LIB}
                      ${SECURITY_LINK_LIB}
)

if(${CASSOBEE_BENCHMARK_ENABLE})
    target_link_libraries(bee benchmark benchmark_main)
endif()

add_common_flags(bee)