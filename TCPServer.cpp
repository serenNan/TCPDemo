#include "TCPServer.h"
#include <QHostAddress>
#include <QTextCodec>

TCPServer::TCPServer(QObject *parent)
    : QObject(parent), server(new QTcpServer(this))
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
    if (server->isListening()) {
        stopServer();
    }
    
    if (server->listen(QHostAddress::Any, port)) {
        emit serverStarted(port);
        return true;
    } else {
        QString errorMsg;
        switch (server->serverError()) {
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
    if (server->isListening()) {
        // 断开所有客户端连接
        for (QTcpSocket *client : clients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->disconnectFromHost();
                if (!client->waitForDisconnected(1000)) {
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
    if (!server->isListening() || clients.isEmpty()) {
        return;
    }
    
    // 默认使用UTF-8编码发送消息
    QByteArray data = message.toUtf8();
    
    for (QTcpSocket *client : clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(data);
        }
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
    while (server->hasPendingConnections()) {
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
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QString clientInfo = getClientInfo(clientSocket);
        
        clients.removeAll(clientSocket);
        clientSocket->deleteLater();
        
        emit clientDisconnected(clientInfo);
    }
}

void TCPServer::onClientReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QByteArray data = clientSocket->readAll();
        
        // 智能检测编码
        QString message = tryDecodeMessage(data);
        
        emit messageReceived(getClientInfo(clientSocket), message);
        
        // 回显消息给发送者，使用UTF-8编码
        QString response = tr("%1").arg(message);
        clientSocket->write(response.toUtf8());
    }
}

// 尝试使用不同编码解码消息
QString TCPServer::tryDecodeMessage(const QByteArray &data)
{
    // 首先尝试UTF-8，这是Linux/Unix系统的标准编码
    QString utf8Message = QString::fromUtf8(data);
    if (!utf8Message.contains(QChar(QChar::ReplacementCharacter))) {
        return utf8Message;
    }
    
    // 如果UTF-8解码有问题，尝试GBK/GB18030（常用于中文Windows系统）
    QTextCodec *gbkCodec = QTextCodec::codecForName("GB18030");
    if (gbkCodec) {
        QString gbkMessage = gbkCodec->toUnicode(data);
        if (!gbkMessage.contains(QChar(QChar::ReplacementCharacter))) {
            return gbkMessage;
        }
    }
    
    // 尝试其他可能的中文编码
    QList<QByteArray> codecs = {"GBK", "GB2312", "Big5"};
    for (const QByteArray &codecName : codecs) {
        QTextCodec *codec = QTextCodec::codecForName(codecName);
        if (codec) {
            QString message = codec->toUnicode(data);
            if (!message.contains(QChar(QChar::ReplacementCharacter))) {
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
    if (address.toString().startsWith("::ffff:")) {
        // 直接从字符串中提取IPv4部分
        addressStr = address.toString().mid(7); // 去掉"::ffff:"前缀
    } else {
        addressStr = address.toString();
    }
    
    return QString("%1:%2").arg(addressStr).arg(socket->peerPort());
} 