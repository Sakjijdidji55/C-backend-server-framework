cmake_minimum_required(VERSION 3.15)
project(CBSF VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录配置
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # 核心：exe输出到bin根目录

# -------------------------- MySQL & Redis 配置（纯相对路径） --------------------------
# MySQL 配置（保留你的原有逻辑）
set(MYSQL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mysql/include)
set(MYSQL_LIB_FILE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mysql/lib/libmysql.lib)
set(MYSQL_DLL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mysql/lib/libmysql.dll)

# Redis 配置（参考FlightServer的链接方式）
set(REDIS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/redis/include)
set(REDIS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/redis/lib)

# 包含Redis头文件 + 设置链接路径（参考FlightServer的link_directories）
include_directories(${REDIS_INCLUDE_DIR})
link_directories(${REDIS_LIB_DIR})

# 检查文件是否存在（仅Windows下检查，Linux使用系统库）
if(WIN32)
    if(NOT EXISTS ${MYSQL_INCLUDE_DIR})
        message(FATAL_ERROR "MySQL头文件目录不存在: ${MYSQL_INCLUDE_DIR}")
    endif()
    if(NOT EXISTS ${MYSQL_LIB_FILE})
        message(FATAL_ERROR "MySQL库文件不存在: ${MYSQL_LIB_FILE}")
    endif()
    if(NOT EXISTS ${MYSQL_DLL_FILE})
        message(FATAL_ERROR "MySQL动态库不存在: ${MYSQL_DLL_FILE}")
    endif()
    # 补充检查Redis库（Windows）
    find_library(HIREDIS_LIB hiredis PATHS ${REDIS_LIB_DIR} REQUIRED)
    if(NOT HIREDIS_LIB)
        message(FATAL_ERROR "Redis库文件不存在: ${REDIS_LIB_DIR}/libhiredis.a/libhiredis.lib")
    endif()
endif()

# 源文件和头文件（优化：移除重复的文件指定，GLOB已包含）
file(GLOB_RECURSE INCLUDE_FILES "include/*.h")
file(GLOB_RECURSE SRC_FILES "src/*.cpp" "main.cpp")
file(GLOB_RECURSE MODELS_FILES "model/*.cpp" "model/*.h")

if(NOT SRC_FILES)
    message(FATAL_ERROR "未找到源文件，请检查目录结构")
endif()

# 创建可执行文件（修正：移除重复的DBConnector文件指定，GLOB已包含）
add_executable(${PROJECT_NAME} ${INCLUDE_FILES} ${SRC_FILES} ${MODELS_FILES})

# ===================== 移动到此处：编译器/平台宏定义（核心修正） =====================
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_MSVC=1)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zi /EHsc)
    target_link_options(${PROJECT_NAME} PRIVATE /DEBUG)
elseif(MINGW)
    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_MINGW=1)
    # 移除 -fno-exceptions（保留异常支持），仅保留必要选项
    target_compile_options(${PROJECT_NAME} PRIVATE -fexceptions) # 显式启用异常
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_LINUX=1)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_CLANG=1)
    else()
        target_compile_definitions(${PROJECT_NAME} PRIVATE COMPILER_GCC=1)
    endif()
endif()

# 包含头文件目录（相对路径）
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MYSQL_INCLUDE_DIR}
        ${REDIS_INCLUDE_DIR}  # 确保Redis头文件被包含
)

# -------------------------- 跨平台依赖查找（核心修复） --------------------------
if(UNIX)
    # Linux/Mac：查找系统依赖库
    find_package(OpenSSL REQUIRED)
    find_package(ZLIB REQUIRED)
    # 查找系统Redis库（Linux下优先用系统库）
    find_library(HIREDIS_LIB hiredis REQUIRED)
    # 追加Linux下的头文件（兼容系统库）
    target_include_directories(${PROJECT_NAME} PRIVATE
            ${OPENSSL_INCLUDE_DIR}
            ${ZLIB_INCLUDE_DIR}
    )
endif()

# 编译选项（核心修正：添加MinGW静态链接标准库，解决单独运行exe依赖问题）
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /W4 /WX /permissive- $<$<CONFIG:Debug>:/ZI>
            /wd4200  # MSVC下屏蔽柔性数组警告（C4200号警告）
    )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/INCREMENTAL>
    )
else()
    # GCC/Clang/MinGW编译器（兼容所有版本，移除无效警告选项）
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall                   # 基本警告
            -Wextra                 # 额外警告
            -Wno-pedantic           # 仅关闭pedantic严格检查（解决柔性数组报错，兼容所有MinGW版本）
            -Werror                 # 警告视为错误（保留其他警告的严格性）
            -g                      # 调试信息
            $<$<CONFIG:Release>:-O2> # 发布模式优化
            -static-libstdc++       # 静态链接C++标准库（核心：摆脱libstdc++-6.dll依赖）
            -static-libgcc          # 静态链接GCC库（核心：摆脱libgcc_s_seh-1.dll依赖）
    )
    # 链接选项补充：静态链接pthread，摆脱libwinpthread-1.dll依赖
    target_link_options(${PROJECT_NAME} PRIVATE
            -static-libstdc++
            -static-libgcc
            $<$<BOOL:${WIN32}>:-static> # 仅Windows下全静态链接，Linux保留动态链接glibc
            $<$<BOOL:${UNIX}>:-rdynamic> # 仅添加：Linux崩溃回溯需要的符号表导出
    )
endif()

# 链接库（核心：补充Redis库 + 修复库顺序，参考FlightServer）
if(WIN32)
    # 复制MySQL DLL到bin根目录（保留你的原有逻辑）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MYSQL_DLL_FILE}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
            COMMENT "Copying libmysql.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    # 先链接业务库，再链接系统库（Windows 链接器对顺序敏感）
    target_link_libraries(${PROJECT_NAME} PRIVATE
            # 业务库
            ${MYSQL_LIB_FILE}  # MySQL库
            ${HIREDIS_LIB}     # Redis库

            # Windows 网络库（必须放在 hiredis 后面）
            ws2_32
            mswsock         # 补充Redis依赖的扩展网络库
            wsock32         # 兼容旧版Socket
            iphlpapi        # 网络辅助库

            # 仅添加：崩溃处理所需的dbghelp库
            dbghelp

            # 其他系统库
            kernel32
            user32
            gdi32
            winspool
            shell32
            ole32
            oleaut32
            uuid
            comdlg32
            advapi32
    )
else()
    # Linux：补充Redis库 + 保留原有依赖
    target_link_libraries(${PROJECT_NAME} PRIVATE
            pthread
            mysqlclient
            ${OPENSSL_LIBRARIES}  # 解决SSL/EVP加密相关错误
            ${ZLIB_LIBRARIES}     # 解决zlib/ZSTD压缩相关错误
            dl                    # 解决dlopen相关错误
            rt                    # 解决实时库相关依赖
            ${HIREDIS_LIB}        # Linux下Redis库（系统库/libhiredis.a）
    )
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

enable_testing()

# 打印配置信息（补充Redis相关 + 新增编译器信息）
message(STATUS "======================================")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "系统平台: ${CMAKE_SYSTEM_NAME}")
if(MSVC)
    message(STATUS "编译器: MSVC (${CMAKE_CXX_COMPILER_VERSION})")
elseif(MINGW)
    message(STATUS "编译器: MinGW/GCC (${CMAKE_CXX_COMPILER_VERSION})")
elseif(UNIX)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "编译器: Clang (${CMAKE_CXX_COMPILER_VERSION})")
    else()
        message(STATUS "编译器: GCC (${CMAKE_CXX_COMPILER_VERSION})")
    endif()
endif()
message(STATUS "MySQL头文件路径(相对): ${MYSQL_INCLUDE_DIR}")
if(WIN32)
    message(STATUS "MySQL库文件路径(相对): ${MYSQL_LIB_FILE}")
    message(STATUS "Redis库文件路径: ${HIREDIS_LIB}")
    message(STATUS "静态链接MinGW标准库: ON")
    message(STATUS "libmysql.dll复制路径: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else()
    message(STATUS "OpenSSL库路径: ${OPENSSL_LIBRARIES}")
    message(STATUS "Zlib库路径: ${ZLIB_LIBRARIES}")
    message(STATUS "Redis库路径: ${HIREDIS_LIB}")
endif()
message(STATUS "======================================")