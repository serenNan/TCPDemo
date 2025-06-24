cmake_minimum_required(VERSION 3.10)
project(TCPDemo VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 添加编译选项
add_compile_options(-Wall -Wextra)

# 查找Qt包
find_package(Qt6 COMPONENTS Core Widgets Network Core5Compat Gui REQUIRED)

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
    TCPClient.cpp
    TCPClient.h
    TCPServer.cpp
    TCPServer.h
)

# TCP演示程序可执行文件（二合一模式）
add_executable(TCPDemo ${TCP_DEMO_SOURCES})
target_link_libraries(TCPDemo PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Core5Compat)
