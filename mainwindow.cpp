#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QHostAddress>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientSocket(new QTcpSocket(this)),
    server(new QTcpServer(this)),
    currentMode(ClientMode)
{
    ui->setupUi(this);
    
    // 初始化UI
    updateUI();
    
    // 设置默认值
    ui->serverAddressEdit->setText("127.0.0.1");
    ui->portEdit->setText("8888");
    ui->modeComboBox->setCurrentText("客户端");
    
    setupConnections();
}

MainWindow::~MainWindow()
{
    // 确保在析构时正确关闭所有连接
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->disconnectFromHost();
        if (!clientSocket->waitForDisconnected(3000)) {
            clientSocket->abort();
        }
    }
    
    if (server->isListening()) {
        // 断开所有客户端连接
        for (QTcpSocket *client : serverClients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->disconnectFromHost();
                if (!client->waitForDisconnected(1000)) {
                    client->abort();
                }
            }
        }
        serverClients.clear();
        server->close();
    }
    
    delete ui;
}

void MainWindow::setupConnections()
{
    // 客户端信号连接
    connect(clientSocket, &QTcpSocket::connected, this, &MainWindow::onClientSocketConnected);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientSocketDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onClientSocketReadyRead);
    connect(clientSocket, &QTcpSocket::errorOccurred, this, &MainWindow::onClientSocketError);
    
    // 服务端信号连接
    connect(server, &QTcpServer::newConnection, this, &MainWindow::onServerNewConnection);
}

void MainWindow::on_modeComboBox_currentTextChanged(const QString &mode)
{
    if (mode == "客户端") {
        currentMode = ClientMode;
    } else if (mode == "服务端") {
        currentMode = ServerMode;
    }
    updateUI();
}

void MainWindow::on_startButton_clicked()
{
    if (currentMode == ServerMode) {
        int port = ui->portEdit->text().toInt();
        
        // 如果服务器已经在运行，先关闭它
        if (server->isListening()) {
            on_stopButton_clicked();
        }
        
        // 尝试启动服务器
        if (server->listen(QHostAddress::Any, port)) {
            appendToLog(tr("服务器已启动，监听端口: %1").arg(port));
            updateUI();
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
            
            QMessageBox::critical(this, tr("服务器启动失败"), errorMsg);
            appendToLog(tr("服务器启动失败: %1").arg(errorMsg));
        }
    }
}

void MainWindow::on_stopButton_clicked()
{
    if (currentMode == ServerMode && server->isListening()) {
        appendToLog(tr("正在停止服务器..."));
        
        // 断开所有客户端连接
        for (QTcpSocket *client : serverClients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->disconnectFromHost();
                // 给客户端一点时间优雅断开
                if (!client->waitForDisconnected(1000)) {
                    client->abort();
                }
            }
        }
        serverClients.clear();
        
        // 关闭服务器
        server->close();
        appendToLog(tr("服务器已停止"));
        updateUI();
    }
}

void MainWindow::on_connectButton_clicked()
{
    if (currentMode == ClientMode) {
        if (clientSocket->state() == QAbstractSocket::UnconnectedState) {
            QString serverAddress = ui->serverAddressEdit->text();
            int port = ui->portEdit->text().toInt();
            
            appendToLog(tr("正在连接到 %1:%2...").arg(serverAddress).arg(port));
            clientSocket->connectToHost(serverAddress, port);
        } else {
            appendToLog(tr("正在断开连接..."));
            clientSocket->disconnectFromHost();
        }
    }
}

void MainWindow::on_sendButton_clicked()
{
    sendMessage();
}

void MainWindow::on_messageEdit_returnPressed()
{
    sendMessage();
}

void MainWindow::on_actionNewWindow_triggered()
{
    MainWindow *newWindow = new MainWindow();
    newWindow->setAttribute(Qt::WA_DeleteOnClose);
    newWindow->show();
}

void MainWindow::sendMessage()
{
    QString message = ui->messageEdit->text().trimmed();
    if (message.isEmpty()) {
        return;
    }
    
    if (currentMode == ClientMode && clientSocket->state() == QAbstractSocket::ConnectedState) {
        clientSocket->write(message.toUtf8());
        appendToLog(tr("发送: %1").arg(message));
        ui->messageEdit->clear();
    } else if (currentMode == ServerMode && !serverClients.isEmpty()) {
        // 向所有连接的客户端发送消息
        int sentCount = 0;
        for (QTcpSocket *client : serverClients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->write(message.toUtf8());
                sentCount++;
            }
        }
        appendToLog(tr("广播给 %1 个客户端: %2").arg(sentCount).arg(message));
        ui->messageEdit->clear();
    }
}

// 客户端相关槽函数
void MainWindow::onClientSocketConnected()
{
    appendToLog(tr("已连接到服务器"));
    updateUI();
}

void MainWindow::onClientSocketDisconnected()
{
    appendToLog(tr("已断开连接"));
    updateUI();
}

void MainWindow::onClientSocketReadyRead()
{
    QByteArray data = clientSocket->readAll();
    appendToLog(tr("接收: %1").arg(QString::fromUtf8(data)));
}

void MainWindow::onClientSocketError(QAbstractSocket::SocketError socketError)
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
    
    if (socketError != QAbstractSocket::RemoteHostClosedError) {
        QMessageBox::information(this, tr("TCP客户端"), errorMsg);
    }
    appendToLog(errorMsg);
    updateUI();
}

// 服务端相关槽函数
void MainWindow::onServerNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *clientSocket = server->nextPendingConnection();
        serverClients.append(clientSocket);
        
        // 连接客户端信号
        connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onServerClientDisconnected);
        connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onServerClientReadyRead);
        
        appendToLog(tr("新客户端连接: %1:%2")
                   .arg(clientSocket->peerAddress().toString())
                   .arg(clientSocket->peerPort()));
        updateUI();
    }
}

void MainWindow::onServerClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        appendToLog(tr("客户端断开连接: %1:%2")
                   .arg(clientSocket->peerAddress().toString())
                   .arg(clientSocket->peerPort()));
        
        serverClients.removeAll(clientSocket);
        clientSocket->deleteLater();
        updateUI();
    }
}

void MainWindow::onServerClientReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QByteArray data = clientSocket->readAll();
        QString message = QString::fromUtf8(data);
        
        appendToLog(tr("收到来自 %1:%2 的消息: %3")
                   .arg(clientSocket->peerAddress().toString())
                   .arg(clientSocket->peerPort())
                   .arg(message));
        
        // 回显消息给发送者
        QString response = tr("服务器收到: %1").arg(message);
        clientSocket->write(response.toUtf8());
    }
}

void MainWindow::updateUI()
{
    bool isServerMode = (currentMode == ServerMode);
    bool isClientMode = (currentMode == ClientMode);
    bool serverRunning = server->isListening();
    bool clientConnected = (clientSocket->state() == QAbstractSocket::ConnectedState);
    
    // 模式相关控件
    ui->serverAddressEdit->setVisible(isClientMode);
    ui->serverAddressLabel->setVisible(isClientMode);
    ui->startButton->setVisible(isServerMode);
    ui->stopButton->setVisible(isServerMode);
    ui->connectButton->setVisible(isClientMode);
    
    // 启用/禁用控件
    ui->modeComboBox->setEnabled(!serverRunning && !clientConnected);
    ui->portEdit->setEnabled(!serverRunning && !clientConnected);
    ui->serverAddressEdit->setEnabled(!clientConnected);
    
    if (isServerMode) {
        ui->startButton->setEnabled(!serverRunning);
        ui->stopButton->setEnabled(serverRunning);
        ui->sendButton->setEnabled(serverRunning && !serverClients.isEmpty());
        ui->messageEdit->setEnabled(serverRunning && !serverClients.isEmpty());
        
        if (serverRunning) {
            ui->statusLabel->setText(tr("服务器运行中 (客户端: %1)").arg(serverClients.size()));
        } else {
            ui->statusLabel->setText(tr("服务器未启动"));
        }
    } else {
        ui->connectButton->setEnabled(true);
        ui->sendButton->setEnabled(clientConnected);
        ui->messageEdit->setEnabled(clientConnected);
        
        if (clientConnected) {
            ui->connectButton->setText(tr("断开"));
            ui->statusLabel->setText(tr("已连接"));
        } else {
            ui->connectButton->setText(tr("连接"));
            ui->statusLabel->setText(tr("未连接"));
        }
    }
    
    // 更新窗口标题
    QString title = tr("TCP通信工具 - %1").arg(isServerMode ? tr("服务端") : tr("客户端"));
    setWindowTitle(title);
}

void MainWindow::appendToLog(const QString &message)
{
    ui->logTextEdit->append(message);
} 