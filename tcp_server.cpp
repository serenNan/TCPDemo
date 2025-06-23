#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define PORT 8888
#define BUFFER_SIZE 1024

bool running = true;
int server_fd = -1;

// 信号处理函数，用于优雅退出
void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\n接收到退出信号，服务器正在关闭..." << std::endl;
        running = false;
        if (server_fd != -1)
        {
            close(server_fd);
            server_fd = -1;
        }
        exit(0);
    }
}

int main()
{
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 创建socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        std::cerr << "创建socket失败: " << strerror(errno) << std::endl;
        return -1;
    }

    // 设置socket选项，允许地址重用
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "设置SO_REUSEADDR失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }

    // 设置SO_REUSEPORT选项（如果支持）
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        // 某些系统可能不支持SO_REUSEPORT，这不是致命错误
        std::cout << "警告: 无法设置SO_REUSEPORT: " << strerror(errno) << std::endl;
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 绑定socket到指定端口
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "绑定失败: " << strerror(errno) << std::endl;
        std::cerr << "端口 " << PORT << " 可能已被占用，请稍后再试或使用其他端口" << std::endl;
        close(server_fd);
        return -1;
    }

    // 监听连接请求
    if (listen(server_fd, 5) == -1)
    {
        std::cerr << "监听失败: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "TCP服务器已启动，监听端口: " << PORT << std::endl;
    std::cout << "按 Ctrl+C 退出服务器" << std::endl;

    while (running)
    {
        // 接受客户端连接
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1)
        {
            if (errno == EINTR)
            {
                // 被信号中断，检查是否应该退出
                if (!running)
                    break;
                continue;
            }
            std::cerr << "接受连接失败: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "客户端连接成功，IP: " << inet_ntoa(client_addr.sin_addr)
                  << ", 端口: " << ntohs(client_addr.sin_port) << std::endl;

        // 接收和处理客户端消息
        while (running)
        {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);

            if (bytes_read <= 0)
            {
                if (bytes_read == 0)
                {
                    std::cout << "客户端断开连接" << std::endl;
                }
                else
                {
                    std::cerr << "接收数据错误: " << strerror(errno) << std::endl;
                }
                break;
            }

            std::cout << "收到消息: " << buffer << std::endl;

            // 发送响应
            std::string response = "服务器已收到消息: ";
            response += buffer;
            if (send(client_fd, response.c_str(), response.length(), 0) == -1)
            {
                std::cerr << "发送响应失败: " << strerror(errno) << std::endl;
                break;
            }
        }

        close(client_fd);
    }

    // 关闭服务器socket
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
    }
    std::cout << "服务器已关闭" << std::endl;

    return 0;
}