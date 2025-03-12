# libsecurity
file(GLOB LIBSECURITY_SRC ${SOURCE_PATH}/bee/security/*.cpp)
message("libsecurity source: " ${LIBSECURITY_SRC})
add_library(security SHARED ${LIBSECURITY_SRC})
target_include_directories(security
                           PUBLIC
                           
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