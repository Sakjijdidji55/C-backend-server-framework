cmake_minimum_required(VERSION 3.15)
project(CBSF VERSION 1.0.0 LANGUAGES C CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目录配置
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# -------------------------- MySQL配置（纯相对路径） --------------------------
# 基于项目根目录的相对路径（核心：避开绝对中文路径）
set(MYSQL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mysql/include)
set(MYSQL_LIB_FILE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mysql/lib/libmysql.lib)
set(MYSQL_DLL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mysql/lib/libmysql.dll)

# 检查文件是否存在
if(NOT EXISTS ${MYSQL_INCLUDE_DIR})
    message(FATAL_ERROR "MySQL头文件目录不存在: ${MYSQL_INCLUDE_DIR}")
endif()
if(NOT EXISTS ${MYSQL_LIB_FILE})
    message(FATAL_ERROR "MySQL库文件不存在: ${MYSQL_LIB_FILE}")
endif()
if(NOT EXISTS ${MYSQL_DLL_FILE})
    message(FATAL_ERROR "MySQL动态库不存在: ${MYSQL_DLL_FILE}")
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

# 包含头文件目录（相对路径）
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MYSQL_INCLUDE_DIR}
)

# 编译选项（核心修正：添加MinGW静态链接标准库，解决单独运行exe依赖问题）
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /W4 /WX /permissive- $<$<CONFIG:Debug>:/ZI>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/INCREMENTAL>
    )
else()
    # GCC/Clang/MinGW编译器
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall                   # 基本警告
            -Wextra                 # 额外警告
            -Werror                 # 警告视为错误
            -pedantic               # 严格遵循C++标准
            -g                      # 调试信息
            $<$<CONFIG:Release>:-O2> # 发布模式优化
            -static-libstdc++       # 静态链接C++标准库（核心：摆脱libstdc++-6.dll依赖）
            -static-libgcc          # 静态链接GCC库（核心：摆脱libgcc_s_seh-1.dll依赖）
    )
    # 链接选项补充：静态链接pthread，摆脱libwinpthread-1.dll依赖
    target_link_options(${PROJECT_NAME} PRIVATE
            -static-libstdc++
            -static-libgcc
            -static                 # 静态链接所有可静态的库
    )
endif()

# 链接库（核心：用相对路径指定lib文件，避开中文绝对路径）
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            ws2_32          # 网络库
            ${MYSQL_LIB_FILE}  # MySQL库（相对路径）
    )
    # 复制DLL（优化：直接复制到exe同级目录，确保单独运行能找到）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # 确保目标目录存在（兼容Debug/Release）
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
            # 复制libmysql.dll到exe所在目录（核心：单独运行时能找到）
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MYSQL_DLL_FILE}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/
            COMMENT "Copying libmysql.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
            pthread
            mysqlclient
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

# 打印配置信息（补充：输出静态链接和DLL复制路径）
message(STATUS "======================================")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "MySQL头文件路径(相对): ${MYSQL_INCLUDE_DIR}")
message(STATUS "MySQL库文件路径(相对): ${MYSQL_LIB_FILE}")
message(STATUS "静态链接MinGW标准库: ON")
message(STATUS "libmysql.dll复制路径: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
message(STATUS "======================================")