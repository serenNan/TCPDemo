#include "TCPClient.h"
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QImage>
#include <QTextCodec>

TCPClient::TCPClient(QObject *parent) : QObject(parent), clientSocket(new QTcpSocket(this))
{
    // 连接信号和槽
    connect(clientSocket, &QTcpSocket::connected, this, &TCPClient::onSocketConnected);
    connect(clientSocket, &QTcpSocket::disconnected, this, &TCPClient::onSocketDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &TCPClient::onSocketReadyRead);
    connect(clientSocket, &QTcpSocket::errorOccurred, this, &TCPClient::onSocketError);
}

TCPClient::~TCPClient()
{
    disconnectFromServer();
}

void TCPClient::connectToServer(const QString &address, int port)
{
    if (clientSocket->state() == QAbstractSocket::UnconnectedState)
    {
        clientSocket->connectToHost(address, port);
    }
}

void TCPClient::disconnectFromServer()
{
    if (clientSocket->state() != QAbstractSocket::UnconnectedState)
    {
        clientSocket->disconnectFromHost();
        if (clientSocket->state() != QAbstractSocket::UnconnectedState &&
            !clientSocket->waitForDisconnected(3000))
        {
            clientSocket->abort();
        }
    }
}

void TCPClient::sendMessage(const QString &message)
{
    if (clientSocket->state() == QAbstractSocket::ConnectedState)
    {
        // 根据编码设置对消息进行编码
        QByteArray data = encodeMessage(message);
        clientSocket->write(data);
    }
}

QByteArray TCPClient::encodeMessage(const QString &message)
{
    switch (sendEncoding)
    {
    case GBK: {
        QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
        if (gbkCodec)
        {
            return gbkCodec->fromUnicode(message);
        }
        // 如果没有GBK编码支持，回退到UTF-8
        return message.toUtf8();
    }
    case UTF8:
    default:
        return message.toUtf8();
    }
}

bool TCPClient::isConnected() const
{
    return clientSocket->state() == QAbstractSocket::ConnectedState;
}

void TCPClient::onSocketConnected()
{
    emit connected();
}

void TCPClient::onSocketDisconnected()
{
    emit disconnected();
}

void TCPClient::onSocketReadyRead()
{
    QByteArray data = clientSocket->readAll();
    QString message = tryDecodeMessage(data);

    // 检查是否是文件或图片消息
    if (message.startsWith("[FILE]"))
    {
        // 处理文件消息
        processFileMessage(message);
    }
    else if (message.startsWith("[IMAGE]"))
    {
        // 处理图片消息
        processImageMessage(message);
    }
    else
    {
        // 处理普通文本消息
        emit messageReceived(message);
    }
}

// 尝试使用不同编码解码消息
QString TCPClient::tryDecodeMessage(const QByteArray &data)
{
    // 首先尝试UTF-8，这是Linux/Unix系统的标准编码
    QString utf8Message = QString::fromUtf8(data);
    if (!utf8Message.contains(QChar(QChar::ReplacementCharacter)))
    {
        return utf8Message;
    }

    // 如果UTF-8解码有问题，尝试GBK/GB18030（常用于中文Windows系统）
    QTextCodec *gbkCodec = QTextCodec::codecForName("GB18030");
    if (gbkCodec)
    {
        QString gbkMessage = gbkCodec->toUnicode(data);
        if (!gbkMessage.contains(QChar(QChar::ReplacementCharacter)))
        {
            return gbkMessage;
        }
    }

    // 尝试其他可能的中文编码
    QList<QByteArray> codecs = {"GBK", "GB2312", "Big5"};
    for (const QByteArray &codecName : codecs)
    {
        QTextCodec *codec = QTextCodec::codecForName(codecName);
        if (codec)
        {
            QString message = codec->toUnicode(data);
            if (!message.contains(QChar(QChar::ReplacementCharacter)))
            {
                return message;
            }
        }
    }

    // 如果所有尝试都失败，返回系统默认编码的结果
    return QString::fromLocal8Bit(data);
}

void TCPClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorMsg;
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        errorMsg = tr("服务器关闭了连接");
        break;
    case QAbstractSocket::HostNotFoundError:
        errorMsg = tr("找不到服务器。请检查服务器地址。");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        errorMsg = tr("连接被拒绝。请确保服务器正在运行并检查端口号。");
        break;
    case QAbstractSocket::NetworkError:
        errorMsg = tr("网络错误: %1").arg(clientSocket->errorString());
        break;
    default:
        errorMsg = tr("连接错误: %1").arg(clientSocket->errorString());
        break;
    }

    emit errorOccurred(errorMsg);
}

// 文件发送方法实现
bool TCPClient::sendFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        emit errorOccurred(tr("无法打开文件: %1").arg(filePath));
        return false;
    }

    QFileInfo fileInfo(filePath);
    QByteArray fileData = file.readAll();
    file.close();

    // 构建消息: [FILE]文件名|文件大小|文件类型|Base64数据
    QString message = QString("[FILE]%1|%2|%3|")
                          .arg(fileInfo.fileName())
                          .arg(fileData.size())
                          .arg(fileInfo.suffix());

    message += QString(fileData.toBase64());

    // 发送消息
    sendMessage(message);
    return true;
}

// 图片发送方法实现
bool TCPClient::sendImage(const QString &imagePath)
{
    QImage image(imagePath);
    if (image.isNull())
    {
        emit errorOccurred(tr("无法加载图片: %1").arg(imagePath));
        return false;
    }

    QFileInfo fileInfo(imagePath);
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);

    // 保存为PNG格式
    image.save(&buffer, "PNG");

    // 构建消息: [IMAGE]图片名|图片大小|图片类型|Base64数据
    QString message =
        QString("[IMAGE]%1|%2|%3|").arg(fileInfo.fileName()).arg(imageData.size()).arg("PNG");

    message += QString(imageData.toBase64());

    // 发送消息
    sendMessage(message);
    return true;
}

// 添加文件消息处理方法
void TCPClient::processFileMessage(const QString &message)
{
    // 解析文件消息: [FILE]文件名|文件大小|文件类型|Base64数据
    QString content = message.mid(6); // 去掉 [FILE] 前缀
    QStringList parts = content.split("|", Qt::KeepEmptyParts);

    if (parts.size() < 4)
    {
        emit errorOccurred("收到的文件消息格式错误");
        return;
    }

    QString fileName = parts[0];
    qint64 fileSize = parts[1].toLongLong();
    QString fileType = parts[2];
    QString base64Data = parts[3];

    // 发出文件接收信号
    emit fileReceived(fileName, fileSize, fileType, QByteArray::fromBase64(base64Data.toLatin1()));
}

// 添加图片消息处理方法
void TCPClient::processImageMessage(const QString &message)
{
    // 解析图片消息: [IMAGE]图片名|图片大小|图片类型|Base64数据
    QString content = message.mid(7); // 去掉 [IMAGE] 前缀
    QStringList parts = content.split("|", Qt::KeepEmptyParts);

    if (parts.size() < 4)
    {
        emit errorOccurred("收到的图片消息格式错误");
        return;
    }

    QString imageName = parts[0];
    qint64 imageSize = parts[1].toLongLong();
    QString imageType = parts[2];
    QString base64Data = parts[3];

    // 发出图片接收信号
    emit imageReceived(imageName, imageSize, imageType,
                       QByteArray::fromBase64(base64Data.toLatin1()));
}