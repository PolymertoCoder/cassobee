# liblog
file(GLOB LIBLOG_SRC ${SOURCE_PATH}/bee/log/*.cpp)
set(LIBLOG_SRC ${LIBLOG_SRC}
               ${PROTOCOL_PATH}/state/state_log.cpp)
message("liblog source: " ${LIBLOG_SRC})
add_library(log SHARED ${LIBLOG_SRC})
target_include_directories(log
                           PRIVATE
                           ${SOURCE_PATH}/bee/common
                           ${SOURCE_PATH}/bee/io
                           ${SOURCE_PATH}/bee/log
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
find_library(LIBCRYTOPP_PATH
             NAMES cryptopp
             PATHS ${THIRDPARTY_PATH}/cryptopp
             NO_DEFAULT_PATH
)
target_link_libraries(security ${LIBCRYTOPP_PATH})
add_common_flags(security)

# libcli
add_subdirectory(cli)

# libbee
file(GLOB_RECURSE LIBBEE_SRC ${SOURCE_PATH}/bee/*.cpp)
# 排除指定目录下的源文件
file(GLOB_RECURSE LIBBEE_EXCLUSIVE_SRC ${SOURCE_PATH}/bee/cli/*.cpp)
list(REMOVE_ITEM LIBBEE_SRC ${LIBBEE_EXCLUSIVE_SRC})
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