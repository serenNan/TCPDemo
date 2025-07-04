#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QList>
#include <QObject>
#include <QPair>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextCodec>

class TCPServer : public QObject
{
    Q_OBJECT

  public:
    // 编码类型枚举
    enum EncodingType
    {
        UTF8,
        GBK,
        AUTO
    };

    // 消息类型枚举
    enum MessageType
    {
        TextMessage,
        FileMessage,
        ImageMessage
    };

    explicit TCPServer(QObject *parent = nullptr);
    ~TCPServer();

    // 启动服务器
    bool startServer(int port);

    // 停止服务器
    void stopServer();

    // 发送消息给所有客户端
    void broadcastMessage(const QString &message);

    // 发送消息给特定客户端
    void sendMessageToClient(QTcpSocket *client, const QString &message);
    void sendMessageToClient(const QString &clientInfo, const QString &message);

    // 获取服务器状态
    bool isRunning() const;

    // 获取连接的客户端数量
    int clientCount() const;

    // 获取客户端列表
    QList<QPair<QString, QTcpSocket *>> getClientList() const;

    // 设置发送编码
    void setSendEncoding(EncodingType encoding)
    {
        sendEncoding = encoding;
    }

    // 设置接收编码
    void setReceiveEncoding(EncodingType encoding)
    {
        receiveEncoding = encoding;
    }

    // 发送文件方法
    bool sendFile(const QString &filePath);
    bool sendFileToClient(const QString &clientInfo, const QString &filePath);

    // 发送图片方法
    bool sendImage(const QString &imagePath);
    bool sendImageToClient(const QString &clientInfo, const QString &imagePath);

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

    // 文件接收信号
    void fileReceived(const QString &clientInfo, const QString &fileName, qint64 fileSize,
                      const QString &fileType, const QByteArray &fileData);

    // 图片接收信号
    void imageReceived(const QString &clientInfo, const QString &imageName, qint64 imageSize,
                       const QString &imageType, const QByteArray &imageData);

  private slots:
    // 服务端相关槽函数
    void onNewConnection();
    void onClientDisconnected();
    void onClientReadyRead();

  private:
    // 服务端相关
    QTcpServer *server;
    QList<QTcpSocket *> clients;
    EncodingType sendEncoding = GBK;     // 默认使用GBK编码发送
    EncodingType receiveEncoding = AUTO; // 默认自动检测接收编码

    // 获取客户端信息
    QString getClientInfo(QTcpSocket *socket) const;

    // 根据客户端信息查找对应的socket
    QTcpSocket *findClientByInfo(const QString &clientInfo) const;

    // 尝试使用不同编码解码消息
    QString tryDecodeMessage(const QByteArray &data);

    // 根据设置的编码类型对消息进行编码
    QByteArray encodeMessage(const QString &message);

    // 文件消息处理方法
    void processFileMessage(QTcpSocket *socket, const QString &message);

    // 图片消息处理方法
    void processImageMessage(QTcpSocket *socket, const QString &message);
};

#endif // TCPSERVER_H