#include "TCPClient.h"
#include <QHostAddress>
#include <QTextCodec>

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
        // 根据编码设置对消息进行编码
        QByteArray data = encodeMessage(message);
        clientSocket->write(data);
    }
}

QByteArray TCPClient::encodeMessage(const QString &message)
{
    switch (sendEncoding) {
    case GBK: {
        QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
        if (gbkCodec) {
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
    
    // 根据接收编码设置解码消息
    QString message;
    if (receiveEncoding == AUTO) {
        message = tryDecodeMessage(data);
    } else if (receiveEncoding == GBK) {
        QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
        if (gbkCodec) {
            message = gbkCodec->toUnicode(data);
        } else {
            message = QString::fromUtf8(data);
        }
    } else { // UTF8
        message = QString::fromUtf8(data);
    }
    
    emit messageReceived(message);
}

// 尝试使用不同编码解码消息
QString TCPClient::tryDecodeMessage(const QByteArray &data)
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