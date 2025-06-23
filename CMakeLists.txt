cmake_minimum_required(VERSION 3.10)
project(TCPDemo VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加编译选项
add_compile_options(-Wall -Wextra)

# 查找Qt包
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)


# Qt GUI应用
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# TCP演示程序源文件
set(TCP_DEMO_SOURCES
    TCPDemo.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# TCP演示程序可执行文件
add_executable(TCPDemo ${TCP_DEMO_SOURCES})
target_link_libraries(TCPDemo PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)
