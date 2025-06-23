#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>

class TCPClient : public QObject
{
    Q_OBJECT

public:
    explicit TCPClient(QObject *parent = nullptr);
    ~TCPClient();

    // 连接到服务器
    void connectToServer(const QString &address, int port);
    
    // 断开连接
    void disconnectFromServer();
    
    // 发送消息
    void sendMessage(const QString &message);
    
    // 获取连接状态
    bool isConnected() const;
    
    // 获取客户端socket
    QTcpSocket* getSocket() const { return clientSocket; }

signals:
    // 连接状态变化信号
    void connected();
    void disconnected();
    
    // 接收到消息信号
    void messageReceived(const QString &message);
    
    // 错误信号
    void errorOccurred(const QString &errorMessage);

private slots:
    // 客户端相关槽函数
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    // 客户端相关
    QTcpSocket *clientSocket;
};

#endif // TCPCLIENT_H 