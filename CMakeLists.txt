cmake_minimum_required(VERSION 3.10)
project(TCPDemo VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加编译选项
add_compile_options(-Wall -Wextra)

# 查找Qt包
find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)

# 添加可执行文件
add_executable(tcp_server tcp_server.cpp)
add_executable(tcp_client tcp_client.cpp)

# Qt GUI应用
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt客户端源文件
set(QT_CLIENT_SOURCES
    qt_client.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# Qt客户端可执行文件
add_executable(qt_client ${QT_CLIENT_SOURCES})
target_link_libraries(qt_client PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

# 安装目标
install(TARGETS tcp_server tcp_client qt_client
        DESTINATION bin) 