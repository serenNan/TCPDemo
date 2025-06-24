#include "TCPServer.h"
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QImage>
#include <QTextCodec>

TCPServer::TCPServer(QObject *parent) : QObject(parent), server(new QTcpServer(this))
{
    // 连接信号和槽
    connect(server, &QTcpServer::newConnection, this, &TCPServer::onNewConnection);
}

TCPServer::~TCPServer()
{
    stopServer();
}

bool TCPServer::startServer(int port)
{
    if (server->isListening())
    {
        stopServer();
    }

    if (server->listen(QHostAddress::Any, port))
    {
        emit serverStarted(port);
        return true;
    }
    else
    {
        QString errorMsg;
        switch (server->serverError())
        {
        case QAbstractSocket::AddressInUseError:
            errorMsg = tr("端口 %1 已被占用。请尝试其他端口或等待片刻后重试。").arg(port);
            break;
        case QAbstractSocket::SocketAccessError:
            errorMsg = tr("权限不足，无法绑定端口 %1。请尝试使用大于1024的端口。").arg(port);
            break;
        default:
            errorMsg = tr("无法启动服务器: %1").arg(server->errorString());
            break;
        }

        emit errorOccurred(errorMsg);
        return false;
    }
}

void TCPServer::stopServer()
{
    if (server->isListening())
    {
        // 断开所有客户端连接
        for (QTcpSocket *client : clients)
        {
            if (client->state() == QAbstractSocket::ConnectedState)
            {
                client->disconnectFromHost();
                if (!client->waitForDisconnected(1000))
                {
                    client->abort();
                }
            }
        }

        clients.clear();
        server->close();
        emit serverStopped();
    }
}

void TCPServer::broadcastMessage(const QString &message)
{
    if (!server->isListening() || clients.isEmpty())
    {
        return;
    }

    // 根据编码设置对消息进行编码
    QByteArray data = encodeMessage(message);

    for (QTcpSocket *client : clients)
    {
        if (client->state() == QAbstractSocket::ConnectedState)
        {
            client->write(data);
        }
    }
}

void TCPServer::sendMessageToClient(QTcpSocket *client, const QString &message)
{
    if (!server->isListening() || !client || client->state() != QAbstractSocket::ConnectedState)
    {
        return;
    }

    // 根据编码设置对消息进行编码
    QByteArray data = encodeMessage(message);
    client->write(data);
}

void TCPServer::sendMessageToClient(const QString &clientInfo, const QString &message)
{
    QTcpSocket *client = findClientByInfo(clientInfo);
    if (client)
    {
        sendMessageToClient(client, message);
    }
}

QTcpSocket *TCPServer::findClientByInfo(const QString &clientInfo) const
{
    for (QTcpSocket *client : clients)
    {
        if (getClientInfo(client) == clientInfo)
        {
            return client;
        }
    }
    return nullptr;
}

QList<QPair<QString, QTcpSocket *>> TCPServer::getClientList() const
{
    QList<QPair<QString, QTcpSocket *>> clientList;
    for (QTcpSocket *client : clients)
    {
        clientList.append(qMakePair(getClientInfo(client), client));
    }
    return clientList;
}

QByteArray TCPServer::encodeMessage(const QString &message)
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

bool TCPServer::isRunning() const
{
    return server->isListening();
}

int TCPServer::clientCount() const
{
    return clients.size();
}

void TCPServer::onNewConnection()
{
    while (server->hasPendingConnections())
    {
        QTcpSocket *clientSocket = server->nextPendingConnection();
        clients.append(clientSocket);

        // 连接客户端信号
        connect(clientSocket, &QTcpSocket::disconnected, this, &TCPServer::onClientDisconnected);
        connect(clientSocket, &QTcpSocket::readyRead, this, &TCPServer::onClientReadyRead);

        emit clientConnected(getClientInfo(clientSocket));
    }
}

void TCPServer::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket)
    {
        QString clientInfo = getClientInfo(clientSocket);

        clients.removeAll(clientSocket);
        clientSocket->deleteLater();

        emit clientDisconnected(clientInfo);
    }
}

void TCPServer::onClientReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
    {
        return;
    }

    QByteArray data = socket->readAll();
    QString message = tryDecodeMessage(data);
    QString clientInfo = getClientInfo(socket);

    // 检查是否是文件或图片消息
    if (message.startsWith("[FILE]"))
    {
        // 处理文件消息
        processFileMessage(socket, message);
    }
    else if (message.startsWith("[IMAGE]"))
    {
        // 处理图片消息
        processImageMessage(socket, message);
    }
    else
    {
        // 处理普通文本消息
        emit messageReceived(clientInfo, message);
    }
}

// 尝试使用不同编码解码消息
QString TCPServer::tryDecodeMessage(const QByteArray &data)
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

QString TCPServer::getClientInfo(QTcpSocket *socket) const
{
    QHostAddress address = socket->peerAddress();
    QString addressStr;

    // 检查是否是IPv4映射地址
    if (address.toString().startsWith("::ffff:"))
    {
        // 直接从字符串中提取IPv4部分
        addressStr = address.toString().mid(7); // 去掉"::ffff:"前缀
    }
    else
    {
        addressStr = address.toString();
    }

    return QString("%1:%2").arg(addressStr).arg(socket->peerPort());
}

// 文件发送方法实现 - 广播给所有客户端
bool TCPServer::sendFile(const QString &filePath)
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

    // 广播消息给所有客户端
    broadcastMessage(message);
    return true;
}

// 文件发送方法实现 - 发送给特定客户端
bool TCPServer::sendFileToClient(const QString &clientInfo, const QString &filePath)
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

    // 发送消息给特定客户端
    sendMessageToClient(clientInfo, message);
    return true;
}

// 图片发送方法实现 - 广播给所有客户端
bool TCPServer::sendImage(const QString &imagePath)
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

    // 广播消息给所有客户端
    broadcastMessage(message);
    return true;
}

// 图片发送方法实现 - 发送给特定客户端
bool TCPServer::sendImageToClient(const QString &clientInfo, const QString &imagePath)
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

    // 发送消息给特定客户端
    sendMessageToClient(clientInfo, message);
    return true;
}

// 添加文件消息处理方法
void TCPServer::processFileMessage(QTcpSocket *socket, const QString &message)
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
    emit fileReceived(getClientInfo(socket), fileName, fileSize, fileType,
                      QByteArray::fromBase64(base64Data.toLatin1()));
}

// 添加图片消息处理方法
void TCPServer::processImageMessage(QTcpSocket *socket, const QString &message)
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
    emit imageReceived(getClientInfo(socket), imageName, imageSize, imageType,
                       QByteArray::fromBase64(base64Data.toLatin1()));
}