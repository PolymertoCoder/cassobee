# libdatabase
file(GLOB LIBDATABASE_SRC ${SOURCE_PATH}/bee/database/*.cpp)
message("libdatabase source: " ${LIBDATABASE_SRC})
add_library(database SHARED ${LIBDATABASE_SRC})
target_include_directories(database
                           PUBLIC
                           ${SOURCE_PATH}/bee/database
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
#target_link_libraries(database ${LIBREADLINE_PATH})