# cmake to fastbuild bff tool
macro(fb_init)
    # 创建全局配置存储
    set(FB_GLOBAL_CONFIG "" CACHE INTERNAL "Global FastBuild configuration")
    set(FB_TARGETS "" CACHE INTERNAL "List of FastBuild targets")
    
    # 编译器配置
    set(FB_COMPILER_CONFIG "// Compiler configuration\nCompiler('GlobalCompiler')\n{\n")
    set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.Executable = '${CMAKE_CXX_COMPILER}'\n")
    
    # 跨平台标志处理
    if(MSVC)
        set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.CompilerFamily = 'msvc'\n")
    else()
        set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.CompilerFamily = 'gcc'\n")
    endif()
    
    # 多配置支持
    set(FB_COMPILER_CONFIG "${FB_COMPILER_CONFIG} \t.MultiConfig = true\n}\n\n")
    set(FB_GLOBAL_CONFIG "${FB_GLOBAL_CONFIG}${FB_COMPILER_CONFIG}" CACHE INTERNAL "")
    
    # 链接器配置
    set(FB_LINKER_CONFIG "// Linker configuration\nLinker('GlobalLinker')\n{\n")
    set(FB_LINKER_CONFIG "${FB_LINKER_CONFIG} \t.Executable = '${CMAKE_LINKER}'\n}\n\n")
    set(FB_GLOBAL_CONFIG "${FB_GLOBAL_CONFIG}${FB_LINKER_CONFIG}" CACHE INTERNAL "")
endmacro()

# 转换单个CMake目标为FastBuild规则
function(convert_target_to_fb TARGET_NAME)
    # 获取目标类型
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    
    # 跳过INTERFACE库和UTILITY目标
    if(TARGET_TYPE STREQUAL "INTERFACE_LIBRARY" OR TARGET_TYPE STREQUAL "UTILITY")
        return()
    endif()
    
    # 创建ObjectList名称
    set(OBJECT_LIST "${TARGET_NAME}_Obj")
    
    # 获取源文件
    get_target_property(SOURCES ${TARGET_NAME} SOURCES)
    if(NOT SOURCES)
        set(SOURCES "")
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
    
    # 获取输出目录和名称
    get_target_property(OUTPUT_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)
    if(NOT OUTPUT_DIR)
        set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
    endif()
    
    get_target_property(OUTPUT_NAME ${TARGET_NAME} OUTPUT_NAME)
    if(NOT OUTPUT_NAME)
        set(OUTPUT_NAME ${TARGET_NAME})
    endif()
    
    # 创建ObjectList
    set(TARGET_BFF "// Target: ${TARGET_NAME}\nObjectList('${OBJECT_LIST}')\n{\n")
    set(TARGET_BFF "${TARGET_BFF} \t.Compiler = 'GlobalCompiler'\n")
    set(TARGET_BFF "${TARGET_BFF} \t.CompilerOptions = '")
    
    # 添加包含目录
    foreach(INC ${INCLUDES})
        if(IS_ABSOLUTE ${INC})
            set(TARGET_BFF "${TARGET_BFF} /I\"${INC}\"")
        else()
            set(TARGET_BFF "${TARGET_BFF} /I\"${CMAKE_CURRENT_SOURCE_DIR}/${INC}\"")
        endif()
    endforeach()
    
    # 添加预定义宏
    foreach(DEF ${DEFINES})
        set(TARGET_BFF "${TARGET_BFF} /D${DEF}")
    endforeach()
    
    # 添加编译选项
    foreach(OPT ${COMPILE_OPTIONS})
        set(TARGET_BFF "${TARGET_BFF} ${OPT}")
    endforeach()
    
    set(TARGET_BFF "${TARGET_BFF}'\n")
    
    # 添加源文件
    set(TARGET_BFF "${TARGET_BFF} \t.SourceFiles = {\n")
    foreach(SRC ${SOURCES})
        if(IS_ABSOLUTE ${SRC})
            set(TARGET_BFF "${TARGET_BFF} \t\t'${SRC}',\n")
        else()
            set(TARGET_BFF "${TARGET_BFF} \t\t'${CMAKE_CURRENT_SOURCE_DIR}/${SRC}',\n")
        endif()
    endforeach()
    set(TARGET_BFF "${TARGET_BFF} \t}\n}\n\n")
    
    # 创建顶层目标
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
            set(TARGET_BFF "${TARGET_BFF}, '${LIB}_Obj'")
        else()
            # 处理外部库
            set(TARGET_BFF "${TARGET_BFF}, '${LIB}'")
        endif()
    endforeach()
    
    # 添加链接选项
    if(NOT "${LINK_OPTIONS}" STREQUAL "")
        set(TARGET_BFF "${TARGET_BFF} }\n \t.LinkerOptions = '")
        foreach(OPT ${LINK_OPTIONS})
            set(TARGET_BFF "${TARGET_BFF} ${OPT}")
        endforeach()
        set(TARGET_BFF "${TARGET_BFF}'\n")
    else()
        set(TARGET_BFF "${TARGET_BFF} }\n")
    endif()
    
    # 添加输出路径
    set(TARGET_BFF "${TARGET_BFF} \t.Output = '${OUTPUT_DIR}/${OUTPUT_NAME}${CMAKE_EXECUTABLE_SUFFIX}'\n")
    
    set(TARGET_BFF "${TARGET_BFF}}\n\n")
    
    # 保存配置
    set(FB_GLOBAL_CONFIG "${FB_GLOBAL_CONFIG}${TARGET_BFF}" CACHE INTERNAL "")
    list(APPEND FB_TARGETS ${TARGET_NAME})
    set(FB_TARGETS ${FB_TARGETS} CACHE INTERNAL "")
endfunction()

# 生成BFF文件
function(fb_generate_bff FILE_PATH)
    # 生成配置开关
    set(CONFIG_SWITCH "Switch(\"Config\")\n{\n")
    set(CONFIG_SWITCH "${CONFIG_SWITCH} \t.Values = { \"Debug\", \"Release\" }\n")
    set(CONFIG_SWITCH "${CONFIG_SWITCH} \t.Default = \"Debug\"\n}\n\n")
    
    # 生成环境设置
    set(ENV_SETUP "// Environment setup\nSettings\n{\n")
    set(ENV_SETUP "${ENV_SETUP} \t.Environment = {\n")
    set(ENV_SETUP "${ENV_SETUP} \t\t\"PATH=$ENV{PATH}\",\n")
    
    if(MSVC)
        set(ENV_SETUP "${ENV_SETUP} \t\t\"INCLUDE=$ENV{INCLUDE}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LIB=$ENV{LIB}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LIBPATH=$ENV{LIBPATH}\"\n")
    else()
        set(ENV_SETUP "${ENV_SETUP} \t\t\"C_INCLUDE_PATH=$ENV{C_INCLUDE_PATH}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"CPLUS_INCLUDE_PATH=$ENV{CPLUS_INCLUDE_PATH}\",\n")
        set(ENV_SETUP "${ENV_SETUP} \t\t\"LIBRARY_PATH=$ENV{LIBRARY_PATH}\"\n")
    endif()
    
    set(ENV_SETUP "${ENV_SETUP} \t}\n}\n\n")
    
    # 组合所有内容
    set(FULL_BFF "// Auto-generated FastBuild configuration\n")
    set(FULL_BFF "${FULL_BFF}${CONFIG_SWITCH}")
    set(FULL_BFF "${FULL_BFF}${ENV_SETUP}")
    set(FULL_BFF "${FULL_BFF}${FB_GLOBAL_CONFIG}")
    
    # 添加别名目标
    set(FULL_BFF "${FULL_BFF}Alias('all')\n{\n \t.Targets = { ")
    foreach(TARGET ${FB_TARGETS})
        set(FULL_BFF "${FULL_BFF} '${TARGET}',")
    endforeach()
    set(FULL_BFF "${FULL_BFF} }\n}\n")
    
    file(WRITE ${FILE_PATH} "${FULL_BFF}")
    message(STATUS "Generated FastBuild configuration: ${FILE_PATH}")
endfunction()

function(collect_all_targets DIR OUT_VAR)
    # 获取当前目录下的目标
    get_property(DIR_TARGETS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    
    # 添加到输出变量
    foreach(TGT ${DIR_TARGETS})
        list(APPEND ALL_TARGETS ${TGT})
    endforeach()

    # 获取子目录
    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR ${SUBDIRS})
        collect_all_targets("${SUBDIR}" TMP_TARGETS)
        list(APPEND ALL_TARGETS ${TMP_TARGETS})
    endforeach()

    # 返回结果
    set(${OUT_VAR} ${ALL_TARGETS} PARENT_SCOPE)
endfunction()


# 主入口：转换所有目标
function(generate_fastbuild_config)
    fb_init()
    
    # 获取所有目标
    collect_all_targets(${CASSOBEE_ROOT_PATH} ALL_TARGETS)
    
    # 转换每个目标
    foreach(TARGET ${ALL_TARGETS})
        convert_target_to_fb(${TARGET})
    endforeach()
    
    # 生成BFF文件
    fb_generate_bff(${BUILD_PATH}/fastbuild/build.bff)
endfunction()