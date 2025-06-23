#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int main()
{
    int sock_fd = 0;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 创建socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "创建socket失败" << std::endl;
        return -1;
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // 将IP地址从文本转换为二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        std::cerr << "无效的地址/地址不支持" << std::endl;
        close(sock_fd);
        return -1;
    }

    // 连接到服务器
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "连接失败" << std::endl;
        close(sock_fd);
        return -1;
    }

    std::cout << "已连接到服务器 " << SERVER_IP << ":" << PORT << std::endl;

    // 主通信循环
    while (true)
    {
        std::cout << "请输入要发送的消息 (输入'exit'退出): ";
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit")
        {
            std::cout << "正在退出客户端..." << std::endl;
            break;
        }

        // 发送消息到服务器
        send(sock_fd, message.c_str(), message.length(), 0);

        // 接收服务器响应
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = recv(sock_fd, buffer, BUFFER_SIZE, 0);

        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
            {
                std::cout << "服务器关闭了连接" << std::endl;
            }
            else
            {
                std::cerr << "接收数据错误" << std::endl;
            }
            break;
        }

        std::cout << "服务器响应: " << buffer << std::endl;
    }

    // 关闭socket
    close(sock_fd);

    return 0;
}