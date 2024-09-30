# liblog
file(GLOB LIBLOG_SRC ${SOURCE_PATH}/bee/log/*.cpp)
message("liblog source: " ${LIBLOG_SRC})
add_library(log SHARED ${LIBLOG_SRC})
target_include_directories(log
                           PRIVATE
                           ${SOURCE_PATH}/bee/common
                           ${SOURCE_PATH}/bee/io
                           ${SOURCE_PATH}/bee/traits
                           ${SOURCE_PATH}/bee/utility
                           ${SOURCE_PATH}/protocol/
                           ${SOURCE_PATH}/protocol/include
)
add_common_flags(log)

# libsecurity
aux_source_directory(./security LIBSECURITY_SRC)
file(GLOB LIBSECURITY_SRC ./security/*.cpp)
message("libsecurity source: " ${LIBSECURITY_SRC})
add_library(security SHARED ${LIBSECURITY_SRC})
find_library(LIBCRYTOPP_PATH cryptopp ${THIRDPARTY_PATH}/cryptopp)
target_link_libraries(security ${LIBCRYTOPP_PATH})
add_common_flags(security)

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
                           ${SOURCE_PATH}/protocol
                           ${SOURCE_PATH}/protocol/include
)

set(LIBBEE_DEPEND_LIB log security)
add_dependencies(bee ${LIBBEE_DEPEND_LIB})
target_link_libraries(bee ${LIBBEE_DEPEND_LIB})
add_common_flags(bee)