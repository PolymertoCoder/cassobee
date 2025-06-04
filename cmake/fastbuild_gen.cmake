# ========================================================================
# FastBuild Configuration Generator
# Version: 1.1
# Description: Converts CMake targets to FastBuild BFF configuration
# ========================================================================

# 初始化FastBuild生成环境
macro(fb_init)
    # 重置全局配置存储
    unset(FB_GLOBAL_CONFIG CACHE)
    unset(FB_TARGETS CACHE)
    set(FB_GLOBAL_CONFIG "" CACHE INTERNAL "Global FastBuild configuration")
    set(FB_TARGETS "" CACHE INTERNAL "List of FastBuild targets")
    
    # 编译器配置
    set(FB_COMPILER_CONFIG "// Compiler configuration\nCompiler('GlobalCompiler')\n{\n")
    set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.Executable = '${CMAKE_CXX_COMPILER}'\n")
    
    # 跨平台标志处理
    if(MSVC)
        set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.CompilerFamily = 'msvc'\n")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.CompilerFamily = 'clang'\n")
    else()
        set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.CompilerFamily = 'gcc'\n")
    endif()
    
    # 多配置支持
    set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.MultiConfig = true\n}\n\n")
    set(FB_GLOBAL_CONFIG "${FB_GLOBAL_CONFIG}${FB_COMPILER_CONFIG}" CACHE INTERNAL "")
    
    # 链接器配置
    set(FB_LINKER_CONFIG "// Linker configuration\nLinker('GlobalLinker')\n{\n")
    set(FB_LINKER_CONFIG "${FB_LINKER_CONFIG} \t.Executable = '${CMAKE_LINKER}'\n")
    
    # 添加链接器选项
    if(MSVC)
        set(FB_LINKER_CONFIG "${FB_LINKER_CONFIG} \t.Executable = '${CMAKE_LINKER}'\n")
    else()
        set(FB_LINKER_CONFIG "${FB_LINKER_CONFIG} \t.LinkerType = 'gcc'\n")
    endif()
    
    set(FB_LINKER_CONFIG "${FB_LINKER_CONFIG}}\n\n")
    set(FB_GLOBAL_CONFIG "${FB_GLOBAL_CONFIG}${FB_LINKER_CONFIG}" CACHE INTERNAL "")
    
    message(STATUS "FastBuild environment initialized")
endmacro()

# 转换单个CMake目标为FastBuild规则
function(convert_target_to_fb TARGET_NAME)
    # 调试信息
    message(STATUS "Converting target: ${TARGET_NAME}")
    
    # 获取目标类型
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    
    # 跳过不支持的目标类型
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY" OR 
       TARGET_TYPE STREQUAL "UTILITY" OR 
       TARGET_TYPE STREQUAL "MODULE_LIBRARY")
        message(STATUS "  - Skipping unsupported target type: ${TARGET_TYPE}")
        return()
    endif()
    
    # 创建ObjectList名称
    set(OBJECT_LIST "${TARGET_NAME}_Obj")
    
    # 获取源文件
    get_target_property(SOURCES ${TARGET_NAME} SOURCES)
    if(NOT SOURCES)
        message(STATUS "  - No sources found, skipping")
        return()
    endif()
    
    # 获取包含目录
    get_target_property(INCLUDES ${TARGET_NAME} INCLUDE_DIRECTORIES)
    if(NOT INCLUDES)
        set(INCLUDES "")
    endif()
    
    # 获取编译定义
    get_target_property(DEFINES ${TARGET_NAME} COMPILE_DEFINITIONS)
    if(NOT DEFINES)
        set(DEFINES "")
    endif()
    
    # 获取编译选项
    get_target_property(COMPILE_OPTIONS ${TARGET_NAME} COMPILE_OPTIONS)
    if(NOT COMPILE_OPTIONS)
        set(COMPILE_OPTIONS "")
    endif()
    
    # 获取链接选项
    get_target_property(LINK_OPTIONS ${TARGET_NAME} LINK_OPTIONS)
    if(NOT LINK_OPTIONS)
        set(LINK_OPTIONS "")
    endif()
    
    # 获取链接库
    get_target_property(LINK_LIBS ${TARGET_NAME} LINK_LIBRARIES)
    if(NOT LINK_LIBS)
        set(LINK_LIBS "")
    endif()
    
    # 获取目标源目录
    get_target_property(TARGET_SOURCE_DIR ${TARGET_NAME} SOURCE_DIR)
    if(NOT TARGET_SOURCE_DIR)
        set(TARGET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    
    # 处理输出目录和名称
    set(OUTPUT_DIR "${CMAKE_BINARY_DIR}")
    set(OUTPUT_NAME "${TARGET_NAME}")
    set(OUTPUT_PREFIX "")
    set(OUTPUT_SUFFIX "")
    
    # 根据目标类型设置输出路径
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        # 可执行文件
        get_target_property(TMP_OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
        if(TMP_OUTPUT_DIR)
            set(OUTPUT_DIR ${TMP_OUTPUT_DIR})
        elseif(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
            set(OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif()
        set(OUTPUT_SUFFIX ${CMAKE_EXECUTABLE_SUFFIX})
        
    elseif(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
        # 动态库
        get_target_property(TMP_OUTPUT_DIR ${TARGET_NAME} LIBRARY_OUTPUT_DIRECTORY)
        if(TMP_OUTPUT_DIR)
            set(OUTPUT_DIR ${TMP_OUTPUT_DIR})
        elseif(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
            set(OUTPUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
        endif()
        set(OUTPUT_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
        set(OUTPUT_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
        
    elseif(TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        # 静态库
        get_target_property(TMP_OUTPUT_DIR ${TARGET_NAME} ARCHIVE_OUTPUT_DIRECTORY)
        if(TMP_OUTPUT_DIR)
            set(OUTPUT_DIR ${TMP_OUTPUT_DIR})
        elseif(CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
            set(OUTPUT_DIR ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
        endif()
        set(OUTPUT_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
        set(OUTPUT_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
    endif()
    
    # 获取输出名称
    get_target_property(TARGET_OUTPUT_NAME ${TARGET_NAME} OUTPUT_NAME)
    if(TARGET_OUTPUT_NAME)
        set(OUTPUT_NAME ${TARGET_OUTPUT_NAME})
    endif()
    
    # 确保输出目录为绝对路径
    if(NOT IS_ABSOLUTE "${OUTPUT_DIR}")
        set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/${OUTPUT_DIR}")
    endif()
    
    # 规范化路径
    get_filename_component(OUTPUT_DIR "${OUTPUT_DIR}" ABSOLUTE)
    
    # 完整输出路径
    set(FULL_OUTPUT_PATH "${OUTPUT_DIR}/${OUTPUT_PREFIX}${OUTPUT_NAME}${OUTPUT_SUFFIX}")
    file(TO_NATIVE_PATH "${FULL_OUTPUT_PATH}" NATIVE_OUTPUT_PATH)
    
    # 确保输出目录存在
    if(NOT EXISTS "${OUTPUT_DIR}")
        file(MAKE_DIRECTORY "${OUTPUT_DIR}")
    endif()
    
    # ====================================================================
    # 创建ObjectList配置
    # ====================================================================
    set(TARGET_BFF "// Target: ${TARGET_NAME}\nObjectList('${OBJECT_LIST}')\n{\n")
    set(TARGET_BFF "${TARGET_BFF} \t.Compiler = 'GlobalCompiler'\n")
    
    # 构建编译器选项字符串
    set(COMPILER_OPTS "")
    
    # 添加包含目录
    foreach(INC ${INCLUDES})
        if(NOT IS_ABSOLUTE "${INC}")
            # 尝试解析相对路径
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${INC}")
                set(INC "${CMAKE_CURRENT_SOURCE_DIR}/${INC}")
            elseif(EXISTS "${TARGET_SOURCE_DIR}/${INC}")
                set(INC "${TARGET_SOURCE_DIR}/${INC}")
            endif()
        endif()
        
        # 确保路径存在
        if(EXISTS "${INC}")
            file(TO_NATIVE_PATH "${INC}" NATIVE_INC)
            set(COMPILER_OPTS "${COMPILER_OPTS} /I\"${NATIVE_INC}\"")
        else()
            message(WARNING "Include directory not found: ${INC} for target ${TARGET_NAME}")
        endif()
    endforeach()
    
    # 添加预定义宏
    foreach(DEF ${DEFINES})
        # 处理带值的定义
        if(DEF MATCHES "(.+)=(.+)")
            set(COMPILER_OPTS "${COMPILER_OPTS} /D${CMAKE_MATCH_1}=\\\"${CMAKE_MATCH_2}\\\"")
        else()
            set(COMPILER_OPTS "${COMPILER_OPTS} /D${DEF}")
        endif()
    endforeach()
    
    # 添加编译选项
    foreach(OPT ${COMPILE_OPTIONS})
        # 处理包含空格和引号的选项
        if(OPT MATCHES " ")
            set(COMPILER_OPTS "${COMPILER_OPTS} \\\"${OPT}\\\"")
        else()
            set(COMPILER_OPTS "${COMPILER_OPTS} ${OPT}")
        endif()
    endforeach()
    
    set(TARGET_BFF "${TARGET_BFF} \t.CompilerOptions = '${COMPILER_OPTS}'\n")
    
    # 添加源文件
    set(TARGET_BFF "${TARGET_BFF} \t.SourceFiles = {\n")
    foreach(SRC ${SOURCES})
        # 处理源文件路径
        if(IS_ABSOLUTE "${SRC}")
            set(SOURCE_PATH "${SRC}")
        else()
            # 尝试确定源文件的实际位置
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
                set(SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
            elseif(EXISTS "${TARGET_SOURCE_DIR}/${SRC}")
                set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SRC}")
            else()
                # 作为最后的手段，使用目标的源目录
                set(SOURCE_PATH "${TARGET_SOURCE_DIR}/${SRC}")
                if(NOT EXISTS "${SOURCE_PATH}")
                    message(WARNING "Source file not found: ${SRC} for target ${TARGET_NAME}")
                    continue()
                endif()
            endif()
        endif()
        
        # 转换为原生路径格式
        file(REAL_PATH "${SOURCE_PATH}" ABS_SOURCE_PATH)
        file(TO_NATIVE_PATH "${ABS_SOURCE_PATH}" NATIVE_SOURCE_PATH)
        set(TARGET_BFF "${TARGET_BFF} \t\t'${NATIVE_SOURCE_PATH}',\n")
    endforeach()
    set(TARGET_BFF "${TARGET_BFF} \t}\n}\n\n")
    
    # ====================================================================
    # 创建顶层目标配置
    # ====================================================================
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        set(TARGET_BFF "${TARGET_BFF}Executable('${TARGET_NAME}')\n{\n")
    elseif(TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        set(TARGET_BFF "${TARGET_BFF}StaticLibrary('${TARGET_NAME}')\n{\n")
    else()
        set(TARGET_BFF "${TARGET_BFF}DynamicLibrary('${TARGET_NAME}')\n{\n")
    endif()
    
    set(TARGET_BFF "${TARGET_BFF} \t.Linker = 'GlobalLinker'\n")
    set(TARGET_BFF "${TARGET_BFF} \t.Libraries = { '${OBJECT_LIST}'")
    
    # 添加链接库
    foreach(LIB ${LINK_LIBS})
        if(TARGET ${LIB})
            # 处理目标依赖
            get_target_property(LIB_TYPE ${LIB} TYPE)
            if(NOT LIB_TYPE STREQUAL "INTERFACE_LIBRARY")
                set(TARGET_BFF "${TARGET_BFF}, '${LIB}_Obj'")
            endif()
        else()
            # 处理外部库
            set(TARGET_BFF "${TARGET_BFF}, '${LIB}'")
        endif()
    endforeach()
    
    # 添加链接选项
    if(NOT "${LINK_OPTIONS}" STREQUAL "")
        set(TARGET_BFF "${TARGET_BFF} }\n \t.LinkerOptions = '")
        foreach(OPT ${LINK_OPTIONS})
            # 处理包含空格和引号的选项
            if(OPT MATCHES " ")
                set(TARGET_BFF "${TARGET_BFF} \\\"${OPT}\\\"")
            else()
                set(TARGET_BFF "${TARGET_BFF} ${OPT}")
            endif()
        endforeach()
        set(TARGET_BFF "${TARGET_BFF}'\n")
    else()
        set(TARGET_BFF "${TARGET_BFF} }\n")
    endif()
    
    # 添加输出路径
    set(TARGET_BFF "${TARGET_BFF} \t.Output = '${NATIVE_OUTPUT_PATH}'\n")
    
    set(TARGET_BFF "${TARGET_BFF}}\n\n")
    
    # 调试信息
    message(STATUS "  - Type: ${TARGET_TYPE}")
    message(STATUS "  - Output: ${NATIVE_OUTPUT_PATH}")
    message(STATUS "  - Source count: ${SOURCES_COUNT}")
    
    # 保存配置
    set(FB_GLOBAL_CONFIG "${FB_GLOBAL_CONFIG}${TARGET_BFF}" CACHE INTERNAL "")
    list(APPEND FB_TARGETS ${TARGET_NAME})
    set(FB_TARGETS ${FB_TARGETS} CACHE INTERNAL "")
endfunction()

# 生成BFF文件
function(fb_generate_bff FILE_PATH)
    # 生成配置开关 - 使用正确的Switch语法
    set(CONFIG_SWITCH "Switch\n{\n")
    set(CONFIG_SWITCH "${CONFIG_SWITCH} \t.Name = \"Config\"\n")
    set(CONFIG_SWITCH "${CONFIG_SWITCH} \t.Values = { \"Debug\", \"Release\" }\n")
    set(CONFIG_SWITCH "${CONFIG_SWITCH} \t.Default = \"${CMAKE_BUILD_TYPE}\"\n}\n\n")
    
    # 生成环境设置
    set(ENV_SETUP "// Environment setup\nSettings\n{\n")
    set(ENV_SETUP "${ENV_SETUP} \t.Environment = {\n")
    
    # 通用环境变量
    set(ENV_SETUP "${ENV_SETUP} \t\t\"PATH=$ENV{PATH}\",\n")
    
    # 平台特定环境变量
    if(WIN32)
        set(ENV_SETUP "${ENV_SETUP} \t\t\"INCLUDE=$ENV{INCLUDE}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LIB=$ENV{LIB}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LIBPATH=$ENV{LIBPATH}\",\n")
    else()
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LD_LIBRARY_PATH=$ENV{LD_LIBRARY_PATH}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"C_INCLUDE_PATH=$ENV{C_INCLUDE_PATH}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"CPLUS_INCLUDE_PATH=$ENV{CPLUS_INCLUDE_PATH}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LIBRARY_PATH=$ENV{LIBRARY_PATH}\",\n")
    endif()
    
    # 添加CMake特定变量
    set(ENV_SETUP "${ENV_SETUP} \t\t\"CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}\",\n")
    set(ENV_SETUP "${ENV_SETUP} \t\t\"CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}\"\n")
    
    set(ENV_SETUP "${ENV_SETUP} \t}\n}\n\n")
    
    # 组合所有内容
    set(FULL_BFF "// ======================================================\n")
    set(FULL_BFF "${FULL_BFF}// Auto-generated FastBuild configuration\n")
    set(FULL_BFF "${FULL_BFF}// Generated from CMake project: ${PROJECT_NAME}\n")
    set(FULL_BFF "${FULL_BFF}// Generation time: ${TIMESTAMP}\n")
    set(FULL_BFF "${FULL_BFF}// ======================================================\n\n")
    
    set(FULL_BFF "${FULL_BFF}${CONFIG_SWITCH}")
    set(FULL_BFF "${FULL_BFF}${ENV_SETUP}")
    set(FULL_BFF "${FULL_BFF}${FB_GLOBAL_CONFIG}")
    
    # 添加别名目标
    if(FB_TARGETS)
        set(FULL_BFF "${FULL_BFF}Alias('all')\n{\n \t.Targets = { ")
        foreach(TARGET ${FB_TARGETS})
            set(FULL_BFF "${FULL_BFF} '${TARGET}',")
        endforeach()
        set(FULL_BFF "${FULL_BFF} }\n}\n")
    else()
        set(FULL_BFF "${FULL_BFF}// No targets found for alias 'all'\n")
    endif()
    
    # 确保输出目录存在
    get_filename_component(BFF_DIR ${FILE_PATH} DIRECTORY)
    if(NOT EXISTS "${BFF_DIR}")
        file(MAKE_DIRECTORY "${BFF_DIR}")
    endif()
    
    # 写入文件
    file(WRITE ${FILE_PATH} "${FULL_BFF}")
    message(STATUS "Generated FastBuild configuration: ${FILE_PATH}")
endfunction()

# 收集所有目标（避免重复处理目录）
function(collect_all_targets START_DIR OUT_VAR)
    # 使用全局属性避免重复
    get_property(ALREADY_PROCESSED GLOBAL PROPERTY FB_PROCESSED_DIRECTORIES)
    
    # 检查目录是否已处理
    if(${START_DIR} IN_LIST ALREADY_PROCESSED)
        set(${OUT_VAR} "" PARENT_SCOPE)
        return()
    endif()
    
    # 标记目录为已处理
    list(APPEND ALREADY_PROCESSED ${START_DIR})
    set_property(GLOBAL PROPERTY FB_PROCESSED_DIRECTORIES "${ALREADY_PROCESSED}")
    
    # 获取当前目录下的目标
    get_property(DIR_TARGETS DIRECTORY "${START_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    
    # 添加到输出变量
    set(ALL_TARGETS)
    if(DIR_TARGETS)
        list(APPEND ALL_TARGETS ${DIR_TARGETS})
        message(STATUS "Found targets in ${START_DIR}: ${DIR_TARGETS}")
    endif()

    # 获取子目录
    get_property(SUBDIRS DIRECTORY "${START_DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR ${SUBDIRS})
        # 递归处理子目录
        collect_all_targets("${SUBDIR}" SUBDIR_TARGETS)
        if(SUBDIR_TARGETS)
            list(APPEND ALL_TARGETS ${SUBDIR_TARGETS})
        endif()
    endforeach()

    # 返回结果
    set(${OUT_VAR} ${ALL_TARGETS} PARENT_SCOPE)
endfunction()

# 主入口：转换所有目标
function(generate_fastbuild_config)
    # 获取当前时间戳
    string(TIMESTAMP TIMESTAMP "%Y-%m-%d %H:%M:%S")
    
    # 重置已处理目录列表
    set_property(GLOBAL PROPERTY FB_PROCESSED_DIRECTORIES "")
    
    # 初始化FastBuild环境
    fb_init()
    
    # 获取所有目标
    collect_all_targets(${PROJECT_SOURCE_DIR} ALL_TARGETS)
    
    if(NOT ALL_TARGETS)
        message(WARNING "No targets found for FastBuild conversion")
        return()
    endif()
    
    # 去重
    list(REMOVE_DUPLICATES ALL_TARGETS)
    list(LENGTH ALL_TARGETS ALL_TARGETS_COUNT)
    message(STATUS "Found ${ALL_TARGETS_COUNT} unique targets for conversion")
    
    # 转换每个目标
    foreach(TARGET ${ALL_TARGETS})
        convert_target_to_fb(${TARGET})
    endforeach()
    
    # 生成BFF文件
    set(BFF_OUTPUT_PATH "${CMAKE_BINARY_DIR}/fastbuild/build.bff")
    fb_generate_bff(${BFF_OUTPUT_PATH})
    
    message(STATUS "FastBuild configuration generated successfully")
endfunction()