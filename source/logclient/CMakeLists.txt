file(GLOB_RECURSE LOGCLIENT_SRC ${SOURCE_PATH}/logclient/*.cpp)

set(LOGCLIENT_SRC ${LOGCLIENT_SRC}
                  ${SOURCE_PATH}/protocol/state/state_log.cpp
                  ${SOURCE_PATH}/protocol/source/remotelog.cpp
)
message("logclient source: " ${LOGCLIENT_SRC})

add_library(logclient   SHARED ${LOGCLIENT_SRC})
add_library(logclient_r SHARED ${LOGCLIENT_SRC})

set(LOGCLIENT_PUBLIC_INCLUDES ${SOURCE_PATH}/logclient)

set(LOGCLIENT_PRIVATE_INCLUDES ${SOURCE_PATH}/protocol
                               ${SOURCE_PATH}/protocol/include
)

target_include_directories(logclient   PUBLIC   ${LOGCLIENT_PUBLIC_INCLUDES} PRIVATE ${LOGCLIENT_PRIVATE_INCLUDES})
target_include_directories(logclient_r PUBLIC ${LOGCLIENT_PUBLIC_INCLUDES} PRIVATE ${LOGCLIENT_PRIVATE_INCLUDES})

target_link_libraries(logclient   PRIVATE bee)
target_link_libraries(logclient_r PRIVATE bee_r)

target_compile_definitions(logclient_r PRIVATE -D_REENTRANT)

#add_common_flags(logclient)