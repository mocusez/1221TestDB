cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(QueryEnginePushModel VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加源文件
set(SOURCES
    main.cpp
)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 创建可执行文件
add_executable(QueryEnginePushModel ${SOURCES})
add_executable(test_mvcc test/test_mvcc.cpp)