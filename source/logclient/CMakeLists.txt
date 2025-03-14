file(GLOB_RECURSE LOGCLIENT_SRC ${SOURCE_PATH}/logclient/*.cpp)

set(LOGCLIENT_SRC ${LOGCLIENT_SRC}
                  ${SOURCE_PATH}/protocol/state/state_log.cpp
                  ${SOURCE_PATH}/protocol/source/log_event.cpp
                  ${SOURCE_PATH}/protocol/source/remotelog.cpp
)
message("logclient source: " ${LOGCLIENT_SRC})

add_library(logclient SHARED ${LOGCLIENT_SRC})

target_include_directories(logclient
                           PUBLIC
                           ${SOURCE_PATH}/logclient
                           PRIVATE
                           ${SOURCE_PATH}/protocol
                           ${SOURCE_PATH}/protocol/include
)

target_link_libraries(logclient PRIVATE bee)

#add_common_flags(logclient)