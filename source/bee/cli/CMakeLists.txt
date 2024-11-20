# libcli
file(GLOB LIBCLI_SRC ${SOURCE_PATH}/bee/cli/*.cpp)
message("libcli source: " ${LIBCLI_SRC})
add_library(cli SHARED ${LIBCLI_SRC})
target_include_directories(cli
                           PUBLIC
                           ${THIRDPARTY_PATH}/readline
                           ${THIRDLIB_OUTPUT_PATH}/readline/include
                           ${SOURCE_PATH}/bee/cli
                           PRIVATE
                           ${SOURCE_PATH}/bee/common
                           ${SOURCE_PATH}/bee/io
                           ${SOURCE_PATH}/bee/log
                           ${SOURCE_PATH}/bee/traits
                           ${SOURCE_PATH}/bee/utility
                           ${SOURCE_PATH}/bee/thread
                           ${SOURCE_PATH}/protocol
                           ${SOURCE_PATH}/protocol/include
)
target_link_libraries(cli ${LIBREADLINE_PATH} ncurses)