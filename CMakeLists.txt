cmake_minimum_required(VERSION 3.15)  # 降低版本要求以提高兼容性
project(backend_server VERSION 1.0.0 LANGUAGES CXX)  # 规范项目名称和版本

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # 禁用编译器扩展

# 输出目录配置
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 源文件和头文件
file(GLOB_RECURSE INCLUDE_FILES "include/*.h")
file(GLOB_RECURSE SRC_FILES "src/*.cpp" "main.cpp")

# 确保源文件列表不为空
if(NOT SRC_FILES)
    message(FATAL_ERROR "未找到源文件，请检查目录结构")
endif()

# 创建可执行文件
add_executable(${PROJECT_NAME} ${INCLUDE_FILES} ${SRC_FILES})

# 包含头文件目录
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 编译选项
if(MSVC)
    # Windows MSVC编译器
    target_compile_options(${PROJECT_NAME} PRIVATE
            /W4                     # 启用大部分警告
            /WX                     # 将警告视为错误
            /permissive-            # 启用严格模式
            $<$<CONFIG:Debug>:/ZI>  # 调试信息
    )
    target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:/INCREMENTAL>
    )
else()
    # GCC/Clang编译器
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall                   # 基本警告
            -Wextra                 # 额外警告
            -Werror                 # 警告视为错误
            -pedantic               # 严格遵循C++标准
            -g                      # 调试信息
            $<$<CONFIG:Release>:-O2> # 发布模式优化
    )
endif()

# 跨平台网络库链接
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)  # Windows套接字库
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)  # Linux线程库
endif()

# 安装配置
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

# 可选：添加测试目标
enable_testing()
# add_subdirectory(tests)  # 若有测试目录可启用

# 打印配置 summary
message(STATUS "======================================")
message(STATUS "项目名称: ${PROJECT_NAME}")
message(STATUS "项目版本: ${PROJECT_VERSION}")
message(STATUS "C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "安装前缀: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "======================================")