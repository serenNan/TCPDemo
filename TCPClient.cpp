#include "TCPClient.h"
#include <QHostAddress>

TCPClient::TCPClient(QObject *parent)
    : QObject(parent), clientSocket(new QTcpSocket(this))
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
    if (clientSocket->state() == QAbstractSocket::UnconnectedState) {
        clientSocket->connectToHost(address, port);
    }
}

void TCPClient::disconnectFromServer()
{
    if (clientSocket->state() != QAbstractSocket::UnconnectedState) {
        clientSocket->disconnectFromHost();
        if (clientSocket->state() != QAbstractSocket::UnconnectedState && !clientSocket->waitForDisconnected(3000)) {
            clientSocket->abort();
        }
    }
}

void TCPClient::sendMessage(const QString &message)
{
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->write(message.toUtf8());
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
    emit messageReceived(QString::fromUtf8(data));
}

void TCPClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorMsg;
    switch (socketError) {
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