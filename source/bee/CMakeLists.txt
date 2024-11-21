# liblog
file(GLOB LIBLOG_SRC ${SOURCE_PATH}/bee/log/*.cpp)
set(LIBLOG_SRC ${LIBLOG_SRC}
               ${PROTOCOL_PATH}/state/state_log.cpp
               ${PROTOCOL_PATH}/source/log_event.cpp
               ${PROTOCOL_PATH}/source/remotelog.cpp
)
message("liblog source: " ${LIBLOG_SRC})
add_library(log SHARED ${LIBLOG_SRC})
target_include_directories(log
                           PUBLIC
                           ${SOURCE_PATH}/bee/log
                           PRIVATE
                           ${SOURCE_PATH}/bee/common
                           ${SOURCE_PATH}/bee/io
                           ${SOURCE_PATH}/bee/traits
                           ${SOURCE_PATH}/bee/utility
                           ${SOURCE_PATH}/bee/thread
                           ${SOURCE_PATH}/protocol/
                           ${SOURCE_PATH}/protocol/include
)
add_common_flags(log)

# libsecurity
file(GLOB LIBSECURITY_SRC ${SOURCE_PATH}/bee/security/*.cpp)
message("libsecurity source: " ${LIBSECURITY_SRC})
add_library(security SHARED ${LIBSECURITY_SRC})
target_include_directories(security
                           PUBLIC
                           ${THIRDPARTY_PATH}/cryptopp
                           ${SOURCE_PATH}/bee/security
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
target_link_libraries(security ${LIBCRYPTOPP_PATH})
add_common_flags(security)

# libcli
add_subdirectory(cli)

# libbee
file(GLOB_RECURSE LIBBEE_SRC ${SOURCE_PATH}/bee/*.cpp)
# 排除指定目录下的源文件
list(FILTER LIBBEE_SRC EXCLUDE REGEX "${SOURCE_PATH}/bee/log/.*")
list(FILTER LIBBEE_SRC EXCLUDE REGEX "${SOURCE_PATH}/bee/security/.*")
list(FILTER LIBBEE_SRC EXCLUDE REGEX "${SOURCE_PATH}/bee/cli/.*")
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
)

set(LIBBEE_DEPEND_LIB log security)
add_dependencies(bee ${LIBBEE_DEPEND_LIB})
target_link_libraries(bee ${LIBBEE_DEPEND_LIB})
add_common_flags(bee)