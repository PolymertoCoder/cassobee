# libcmysql
set(LIBCMYSQL_SRC cmysql.cpp)
message("libcmysql source: " ${LIBCMYSQL_SRC})
add_library(cmysql SHARED ${LIBCMYSQL_SRC})
target_include_directories(cmysql
                           PUBLIC
                           ${SOURCE_PATH}/bee/database/
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

target_link_libraries(cmysql mysqlcppconn)

add_common_flags(cmysql)