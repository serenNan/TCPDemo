#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class TCPServer : public QObject
{
    Q_OBJECT

public:
    explicit TCPServer(QObject *parent = nullptr);
    ~TCPServer();

    // 启动服务器
    bool startServer(int port);
    
    // 停止服务器
    void stopServer();
    
    // 发送消息给所有客户端
    void broadcastMessage(const QString &message);
    
    // 获取服务器状态
    bool isRunning() const;
    
    // 获取连接的客户端数量
    int clientCount() const;

signals:
    // 服务器状态变化信号
    void serverStarted(int port);
    void serverStopped();
    
    // 客户端连接变化信号
    void clientConnected(const QString &clientInfo);
    void clientDisconnected(const QString &clientInfo);
    
    // 接收到消息信号
    void messageReceived(const QString &clientInfo, const QString &message);
    
    // 错误信号
    void errorOccurred(const QString &errorMessage);

private slots:
    // 服务端相关槽函数
    void onNewConnection();
    void onClientDisconnected();
    void onClientReadyRead();

private:
    // 服务端相关
    QTcpServer *server;
    QList<QTcpSocket*> clients;
    
    // 获取客户端信息
    QString getClientInfo(QTcpSocket *socket) const;
};

#endif // TCPSERVER_H 