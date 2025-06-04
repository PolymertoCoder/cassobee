# libbee excluded source files
set(LIBBEE_EXCLUDE_SRC "")

# security module
set(SECURITY_LINK_LIB ${LIBCRYPTOPP_PATH})
set(SECURITY_INCLUDE ${THIRDPARTY_PATH}/cryptopp)

# command line module
set(CLI_LINK_LIB ${LIBREADLINE_PATH} ncurses)
set(CLI_INCLUDE ${THIRDPARTY_PATH}/readline
                ${THIRDLIB_OUTPUT_PATH}/readline/include
)

# database module
set(CMYSQL_LINK_LIB mysqlcppconn)

# meta module
if(${CASSOBEE_REFLECTION_ENABLE})
    # 查找LLVM和Clang
    find_package(LLVM REQUIRED CONFIG)
    find_package(Clang REQUIRED CONFIG)

    find_library(LIBCLANG REQUIRED
                 NAMES clang libclang
                 PATHS /usr/lib/x86_64-linux-gnu
    )
    set(META_LINK_LIB ${LIBCLANG})
else()
    # 排除反射相关的源文件
    file(GLOB META_SRC "${SOURCE_PATH}/bee/meta/*.cpp")
    list(APPEND LIBBEE_EXCLUDE_SRC ${META_SRC})
endif()

# libbee
file(GLOB_RECURSE LIBBEE_SRC ${SOURCE_PATH}/bee/*.cpp)
list(REMOVE_ITEM LIBBEE_SRC ${LIBBEE_EXCLUDE_SRC})

add_library(bee SHARED ${LIBBEE_SRC})
add_library(bee_r SHARED ${LIBBEE_SRC})

set(BEE_PUBLIC_INCLUDES ${SOURCE_PATH}/bee/common
                        ${SOURCE_PATH}/bee/io
                        ${SOURCE_PATH}/bee/log
                        ${SOURCE_PATH}/bee/traits
                        ${SOURCE_PATH}/bee/utility
                        ${SOURCE_PATH}/bee/format
                        ${SOURCE_PATH}/bee/thread
                        ${SOURCE_PATH}/bee/security
                        ${SOURCE_PATH}/bee/cli
                        ${SOURCE_PATH}/bee/database
                        ${SOURCE_PATH}/bee/database/mysql
                        ${SOURCE_PATH}/bee/database/sqlite
                        ${SOURCE_PATH}/bee/http
                        ${SOURCE_PATH}/bee/http/servlets
                        ${SOURCE_PATH}/bee/meta
                        ${SOURCE_PATH}/protocol
                        ${SOURCE_PATH}/protocol/include
)

set(BEE_PRIVATE_INCLUDES ${CLI_INCLUDE}
                         ${SECURITY_INCLUDE}
                         ${LLVM_INCLUDE_DIRS}
                         ${CLANG_INCLUDE_DIRS}
)

set(BEE_LINK_LIBS ${CLI_LINK_LIB}
                  ${SECURITY_LINK_LIB}
                  ${CMYSQL_LINK_LIB}
                  ${META_LINK_LIB}
)

target_include_directories(bee   PUBLIC ${BEE_PUBLIC_INCLUDES} PRIVATE ${BEE_PRIVATE_INCLUDES})
target_include_directories(bee_r PUBLIC ${BEE_PUBLIC_INCLUDES} PRIVATE ${BEE_PRIVATE_INCLUDES})

target_link_libraries(bee   ${BEE_LINK_LIBS})
target_link_libraries(bee_r ${BEE_LINK_LIBS} atomic)

target_compile_definitions(bee_r PRIVATE -D_REENTRANT)

if(${CASSOBEE_BENCHMARK_ENABLE})
    target_link_libraries(bee benchmark benchmark_main)
    target_link_libraries(bee_r benchmark benchmark_main)
endif()

add_common_flags(bee)