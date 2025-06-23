#include "TCPServer.h"
#include <QHostAddress>

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
    
    for (QTcpSocket *client : clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(message.toUtf8());
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
        QString message = QString::fromUtf8(data);
        
        emit messageReceived(getClientInfo(clientSocket), message);
        
        // 回显消息给发送者
        QString response = tr("%1").arg(message);
        clientSocket->write(response.toUtf8());
    }
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