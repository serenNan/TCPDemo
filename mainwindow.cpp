#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), client(new TCPClient(this)),
      server(new TCPServer(this)), currentMode(ClientMode)
{
    ui->setupUi(this);

    // 初始化UI
    updateUI();

    // 设置默认值
    ui->serverAddressEdit->setText("127.0.0.1");
    ui->portEdit->setText("8888");
    ui->modeComboBox->setCurrentText("客户端");

    // 默认选择GBK编码发送，自动检测接收
    ui->sendEncodingComboBox->setCurrentIndex(1);    // GBK
    ui->receiveEncodingComboBox->setCurrentIndex(0); // 自动检测

    // 初始化编码设置
    updateEncodingSettings();

    setupConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    // 客户端信号连接
    connect(client, &TCPClient::connected, this, &MainWindow::onClientConnected);
    connect(client, &TCPClient::disconnected, this, &MainWindow::onClientDisconnected);
    connect(client, &TCPClient::messageReceived, this, &MainWindow::onClientMessageReceived);
    connect(client, &TCPClient::errorOccurred, this, &MainWindow::onClientError);

    // 服务端信号连接
    connect(server, &TCPServer::serverStarted, this, &MainWindow::onServerStarted);
    connect(server, &TCPServer::serverStopped, this, &MainWindow::onServerStopped);
    connect(server, &TCPServer::clientConnected, this, &MainWindow::onServerClientConnected);
    connect(server, &TCPServer::clientDisconnected, this, &MainWindow::onServerClientDisconnected);
    connect(server, &TCPServer::messageReceived, this, &MainWindow::onServerMessageReceived);
    connect(server, &TCPServer::errorOccurred, this, &MainWindow::onServerError);
}

void MainWindow::on_modeComboBox_currentTextChanged(const QString &mode)
{
    if (mode == "客户端")
    {
        currentMode = ClientMode;
    }
    else if (mode == "服务端")
    {
        currentMode = ServerMode;
    }
    updateUI();
}

void MainWindow::on_startButton_clicked()
{
    if (currentMode == ServerMode)
    {
        int port = ui->portEdit->text().toInt();
        server->startServer(port);
    }
}

void MainWindow::on_stopButton_clicked()
{
    if (currentMode == ServerMode && server->isRunning())
    {
        appendToLog(tr("正在停止服务器..."));
        server->stopServer();
    }
}

void MainWindow::on_connectButton_clicked()
{
    if (currentMode == ClientMode)
    {
        if (!client->isConnected())
        {
            QString serverAddress = ui->serverAddressEdit->text();
            int port = ui->portEdit->text().toInt();

            appendToLog(tr("正在连接到 %1:%2...").arg(serverAddress).arg(port));
            client->connectToServer(serverAddress, port);
        }
        else
        {
            appendToLog(tr("正在断开连接..."));
            client->disconnectFromServer();
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
    if (message.isEmpty())
    {
        return;
    }

    if (currentMode == ClientMode && client->isConnected())
    {
        client->sendMessage(message);
        appendToLog(tr("发送: %1").arg(message));
        ui->messageEdit->clear();
    }
    else if (currentMode == ServerMode && server->isRunning())
    {
        int targetIndex = ui->targetClientComboBox->currentIndex();

        if (targetIndex == 0)
        {
            // 发送给所有客户端
            server->broadcastMessage(message);
            appendToLog(tr("广播给 %1 个客户端: %2").arg(server->clientCount()).arg(message));
        }
        else
        {
            // 发送给特定客户端
            QString clientInfo = ui->targetClientComboBox->currentText();
            server->sendMessageToClient(clientInfo, message);
            appendToLog(tr("发送给 %1: %2").arg(clientInfo).arg(message));
        }

        ui->messageEdit->clear();
    }
}

// 客户端相关槽函数
void MainWindow::onClientConnected()
{
    appendToLog(tr("已连接到服务器"));
    updateUI();
}

void MainWindow::onClientDisconnected()
{
    appendToLog(tr("已断开连接"));
    updateUI();
}

void MainWindow::onClientMessageReceived(const QString &message)
{
    appendToLog(tr("接收: %1").arg(message));
}

void MainWindow::onClientError(const QString &errorMessage)
{
    if (!errorMessage.contains("服务器关闭了连接"))
    {
        QMessageBox::information(this, tr("TCP客户端"), errorMessage);
    }
    appendToLog(errorMessage);
    updateUI();
}

// 服务端相关槽函数
void MainWindow::onServerStarted(int port)
{
    appendToLog(tr("服务器已启动，监听端口: %1").arg(port));
    updateUI();
}

void MainWindow::onServerStopped()
{
    appendToLog(tr("服务器已停止"));
    updateUI();
}

void MainWindow::onServerClientConnected(const QString &clientInfo)
{
    appendToLog(tr("新客户端连接: %1").arg(clientInfo));
    updateClientList();
    updateUI();
}

void MainWindow::onServerClientDisconnected(const QString &clientInfo)
{
    appendToLog(tr("客户端断开连接: %1").arg(clientInfo));
    updateClientList();
    updateUI();
}

void MainWindow::onServerMessageReceived(const QString &clientInfo, const QString &message)
{
    appendToLog(tr("收到来自 %1 的消息: %2").arg(clientInfo).arg(message));
}

void MainWindow::onServerError(const QString &errorMessage)
{
    QMessageBox::critical(this, tr("服务器错误"), errorMessage);
    appendToLog(tr("服务器错误: %1").arg(errorMessage));
}

void MainWindow::updateUI()
{
    bool isServerMode = (currentMode == ServerMode);
    bool isClientMode = (currentMode == ClientMode);
    bool serverRunning = server->isRunning();
    bool clientConnected = client->isConnected();

    // 模式相关控件
    ui->serverAddressEdit->setVisible(isClientMode);
    ui->serverAddressLabel->setVisible(isClientMode);
    ui->startButton->setVisible(isServerMode);
    ui->stopButton->setVisible(isServerMode);
    ui->connectButton->setVisible(isClientMode);

    // 客户端选择相关控件
    ui->targetClientLabel->setVisible(isServerMode);
    ui->targetClientComboBox->setVisible(isServerMode);

    // 启用/禁用控件
    ui->modeComboBox->setEnabled(!serverRunning && !clientConnected);
    ui->portEdit->setEnabled(!serverRunning && !clientConnected);
    ui->serverAddressEdit->setEnabled(!clientConnected);

    if (isServerMode)
    {
        ui->startButton->setEnabled(!serverRunning);
        ui->stopButton->setEnabled(serverRunning);
        ui->sendButton->setEnabled(serverRunning && server->clientCount() > 0);
        ui->messageEdit->setEnabled(serverRunning && server->clientCount() > 0);
        ui->targetClientComboBox->setEnabled(serverRunning && server->clientCount() > 0);

        if (serverRunning)
        {
            ui->statusLabel->setText(tr("服务器运行中 (客户端: %1)").arg(server->clientCount()));
        }
        else
        {
            ui->statusLabel->setText(tr("服务器未启动"));
        }
    }
    else
    {
        ui->connectButton->setEnabled(true);
        ui->sendButton->setEnabled(clientConnected);
        ui->messageEdit->setEnabled(clientConnected);

        if (clientConnected)
        {
            ui->connectButton->setText(tr("断开"));
            ui->statusLabel->setText(tr("已连接"));
        }
        else
        {
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

void MainWindow::on_sendEncodingComboBox_currentIndexChanged(int index)
{
    updateEncodingSettings();
}

void MainWindow::on_receiveEncodingComboBox_currentIndexChanged(int index)
{
    updateEncodingSettings();
}

void MainWindow::on_targetClientComboBox_currentIndexChanged(int index)
{
    // 可以添加一些逻辑，例如根据选择的客户端更改提示信息等
    if (index == 0)
    {
        ui->messageEdit->setPlaceholderText(tr("输入消息并按回车广播给所有客户端..."));
    }
    else
    {
        QString target = ui->targetClientComboBox->currentText();
        ui->messageEdit->setPlaceholderText(tr("输入消息并按回车发送给 %1...").arg(target));
    }
}

void MainWindow::updateEncodingSettings()
{
    // 获取当前选择的编码
    int sendIndex = ui->sendEncodingComboBox->currentIndex();
    int receiveIndex = ui->receiveEncodingComboBox->currentIndex();

    // 设置客户端编码
    if (sendIndex == 0)
    { // UTF-8
        client->setSendEncoding(TCPClient::UTF8);
    }
    else
    { // GBK
        client->setSendEncoding(TCPClient::GBK);
    }

    if (receiveIndex == 0)
    { // 自动检测
        client->setReceiveEncoding(TCPClient::AUTO);
    }
    else if (receiveIndex == 1)
    { // UTF-8
        client->setReceiveEncoding(TCPClient::UTF8);
    }
    else
    { // GBK
        client->setReceiveEncoding(TCPClient::GBK);
    }

    // 设置服务端编码
    if (sendIndex == 0)
    { // UTF-8
        server->setSendEncoding(TCPServer::UTF8);
    }
    else
    { // GBK
        server->setSendEncoding(TCPServer::GBK);
    }

    if (receiveIndex == 0)
    { // 自动检测
        server->setReceiveEncoding(TCPServer::AUTO);
    }
    else if (receiveIndex == 1)
    { // UTF-8
        server->setReceiveEncoding(TCPServer::UTF8);
    }
    else
    { // GBK
        server->setReceiveEncoding(TCPServer::GBK);
    }

    // 记录编码设置到日志
    QString sendEncoding = ui->sendEncodingComboBox->currentText();
    QString receiveEncoding = ui->receiveEncodingComboBox->currentText();
    appendToLog(tr("编码设置已更新 - 发送: %1, 接收: %2").arg(sendEncoding).arg(receiveEncoding));
}

void MainWindow::updateClientList()
{
    // 保存当前选择的项
    QString currentSelected = ui->targetClientComboBox->currentText();

    // 清空下拉框，但保留"所有客户端"选项
    while (ui->targetClientComboBox->count() > 1)
    {
        ui->targetClientComboBox->removeItem(1);
    }

    // 添加当前连接的客户端
    auto clientList = server->getClientList();
    for (const auto &client : clientList)
    {
        ui->targetClientComboBox->addItem(client.first);
    }

    // 尝试恢复之前选择的项
    int index = ui->targetClientComboBox->findText(currentSelected);
    if (index != -1)
    {
        ui->targetClientComboBox->setCurrentIndex(index);
    }
    else
    {
        ui->targetClientComboBox->setCurrentIndex(0); // 默认选择"所有客户端"
    }
}