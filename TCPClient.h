#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTextCodec>

class TCPClient : public QObject
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
    QTcpSocket *getSocket() const
    {
        return clientSocket;
    }

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

    // 发送图片方法
    bool sendImage(const QString &imagePath);

  signals:
    // 连接状态变化信号
    void connected();
    void disconnected();

    // 接收到消息信号
    void messageReceived(const QString &message);

    // 错误信号
    void errorOccurred(const QString &errorMessage);

    // 文件接收信号
    void fileReceived(const QString &fileName, qint64 fileSize, const QString &fileType,
                      const QByteArray &fileData);

    // 图片接收信号
    void imageReceived(const QString &imageName, qint64 imageSize, const QString &imageType,
                       const QByteArray &imageData);

  private slots:
    // 客户端相关槽函数
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

  private:
    // 客户端相关
    QTcpSocket *clientSocket;
    EncodingType sendEncoding = GBK;     // 默认使用GBK编码发送
    EncodingType receiveEncoding = AUTO; // 默认自动检测接收编码

    // 尝试使用不同编码解码消息
    QString tryDecodeMessage(const QByteArray &data);

    // 根据设置的编码类型对消息进行编码
    QByteArray encodeMessage(const QString &message);

    // 文件消息处理方法
    void processFileMessage(const QString &message);

    // 图片消息处理方法
    void processImageMessage(const QString &message);
};

#endif // TCPCLIENT_H