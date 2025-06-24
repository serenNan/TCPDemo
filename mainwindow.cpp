#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
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
    connect(client, &TCPClient::fileReceived, this, &MainWindow::onClientFileReceived);
    connect(client, &TCPClient::imageReceived, this, &MainWindow::onClientImageReceived);

    // 服务端信号连接
    connect(server, &TCPServer::serverStarted, this, &MainWindow::onServerStarted);
    connect(server, &TCPServer::serverStopped, this, &MainWindow::onServerStopped);
    connect(server, &TCPServer::clientConnected, this, &MainWindow::onServerClientConnected);
    connect(server, &TCPServer::clientDisconnected, this, &MainWindow::onServerClientDisconnected);
    connect(server, &TCPServer::messageReceived, this, &MainWindow::onServerMessageReceived);
    connect(server, &TCPServer::errorOccurred, this, &MainWindow::onServerError);
    connect(server, &TCPServer::fileReceived, this, &MainWindow::onServerFileReceived);
    connect(server, &TCPServer::imageReceived, this, &MainWindow::onServerImageReceived);
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
        appendTimestampedMessage(tr("发送: %1").arg(message), SentMessage);
        ui->messageEdit->clear();
    }
    else if (currentMode == ServerMode && server->isRunning())
    {
        int targetIndex = ui->targetClientComboBox->currentIndex();

        if (targetIndex == 0)
        {
            // 发送给所有客户端
            server->broadcastMessage(message);
            appendTimestampedMessage(
                tr("广播给 %1 个客户端: %2").arg(server->clientCount()).arg(message), SentMessage);
        }
        else
        {
            // 发送给特定客户端
            QString clientInfo = ui->targetClientComboBox->currentText();
            server->sendMessageToClient(clientInfo, message);
            appendTimestampedMessage(tr("发送给 %1: %2").arg(clientInfo).arg(message), SentMessage);
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
    appendTimestampedMessage(tr("接收: %1").arg(message), ReceivedMessage);
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
    appendTimestampedMessage(tr("收到来自 %1 的消息: %2").arg(clientInfo).arg(message),
                             ReceivedMessage);
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

void MainWindow::appendToLog(const QString &message, MessageType type)
{
    QString coloredMessage;

    switch (type)
    {
    case SystemMessage:
        coloredMessage = QString("<font color='gray'>%1</font>").arg(message);
        break;
    case SentMessage:
        coloredMessage = QString("<font color='black'>%1</font>").arg(message);
        break;
    case ReceivedMessage:
        coloredMessage = QString("<font color='black'>%1</font>").arg(message);
        break;
    case TimeStamp:
        coloredMessage = QString("<font color='blue'>%1</font>").arg(message);
        break;
    }

    ui->logTextEdit->append(coloredMessage);
}

void MainWindow::appendTimestampedMessage(const QString &message, MessageType type)
{
    // 对于接收的消息，分离系统信息和实际消息内容
    if (type == ReceivedMessage)
    {
        // 查找消息内容的起始位置
        int colonPos = message.lastIndexOf(": ");
        if (colonPos != -1)
        {
            // 分离系统信息和实际消息内容
            QString sysInfo = message.left(colonPos + 2); // 包含冒号和空格
            QString actualMessage = message.mid(colonPos + 2);

            // 添加时间戳和系统信息在同一行
            QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ");
            QString combinedInfo = timestamp + sysInfo;
            ui->logTextEdit->append(
                QString("<font color='blue'>%1</font><font color='gray'>%2</font>")
                    .arg(timestamp)
                    .arg(sysInfo));

            // 实际消息内容单独一行显示
            appendToLog(actualMessage, type);
        }
        else
        {
            // 如果无法分离，则使用原来的方式显示
            QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]");
            appendToLog(timestamp, TimeStamp);
            appendToLog(message, type);
        }
    }
    // 对于发送的消息，也进行类似处理
    else if (type == SentMessage)
    {
        int colonPos = message.indexOf(": ");
        if (colonPos != -1)
        {
            // 分离系统信息和实际消息内容
            QString sysInfo = message.left(colonPos + 2); // 包含冒号和空格
            QString actualMessage = message.mid(colonPos + 2);

            // 添加时间戳和系统信息在同一行
            QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ");
            ui->logTextEdit->append(
                QString("<font color='blue'>%1</font><font color='gray'>%2</font>")
                    .arg(timestamp)
                    .arg(sysInfo));

            // 实际消息内容单独一行显示
            appendToLog(actualMessage, type);
        }
        else
        {
            // 如果无法分离，则使用原来的方式显示
            QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]");
            appendToLog(timestamp, TimeStamp);
            appendToLog(message, type);
        }
    }
    else
    {
        // 其他类型的消息保持原样处理
        QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]");
        appendToLog(timestamp, TimeStamp);
        appendToLog(message, type);
    }

    // 添加空行
    ui->logTextEdit->append("");
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

void MainWindow::on_sendFileButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"));
    if (filePath.isEmpty())
    {
        return;
    }

    QFileInfo fileInfo(filePath);

    if (currentMode == ClientMode && client->isConnected())
    {
        if (client->sendFile(filePath))
        {
            appendTimestampedMessage(
                tr("发送文件: %1 (%2 字节)").arg(fileInfo.fileName()).arg(fileInfo.size()),
                SentMessage);
        }
    }
    else if (currentMode == ServerMode && server->isRunning())
    {
        int targetIndex = ui->targetClientComboBox->currentIndex();

        if (targetIndex == 0)
        {
            // 广播文件给所有客户端
            // 由于文件可能较大，这里提示用户确认
            QMessageBox::StandardButton reply =
                QMessageBox::question(this, tr("广播文件"),
                                      tr("确定要将文件 %1 广播给所有 %2 个客户端吗?")
                                          .arg(fileInfo.fileName())
                                          .arg(server->clientCount()),
                                      QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes)
            {
                if (server->sendFile(filePath))
                {
                    appendTimestampedMessage(tr("广播文件给 %1 个客户端: %2 (%3 字节)")
                                                 .arg(server->clientCount())
                                                 .arg(fileInfo.fileName())
                                                 .arg(fileInfo.size()),
                                             SentMessage);
                }
            }
        }
        else
        {
            // 发送给特定客户端
            QString clientInfo = ui->targetClientComboBox->currentText();
            if (server->sendFileToClient(clientInfo, filePath))
            {
                appendTimestampedMessage(tr("发送文件给 %1: %2 (%3 字节)")
                                             .arg(clientInfo)
                                             .arg(fileInfo.fileName())
                                             .arg(fileInfo.size()),
                                         SentMessage);
            }
        }
    }
}

void MainWindow::on_sendImageButton_clicked()
{
    QString imagePath = QFileDialog::getOpenFileName(
        this, tr("选择图片"), "", tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (imagePath.isEmpty())
    {
        return;
    }

    QFileInfo fileInfo(imagePath);

    if (currentMode == ClientMode && client->isConnected())
    {
        if (client->sendImage(imagePath))
        {
            appendTimestampedMessage(tr("发送图片: %1").arg(fileInfo.fileName()), SentMessage);

            // 在消息区域显示图片预览
            QImage image(imagePath);
            if (!image.isNull())
            {
                // 调整图片大小以适应消息区域
                QImage scaledImage = image.scaledToWidth(300, Qt::SmoothTransformation);
                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                buffer.open(QIODevice::WriteOnly);
                scaledImage.save(&buffer, "PNG");
                QString html = QString("<img src='data:image/png;base64,%1' />")
                                   .arg(QString(byteArray.toBase64()));
                ui->logTextEdit->append(html);
            }
        }
    }
    else if (currentMode == ServerMode && server->isRunning())
    {
        // 类似文件发送的逻辑处理
        int targetIndex = ui->targetClientComboBox->currentIndex();

        if (targetIndex == 0)
        {
            // 广播图片给所有客户端
            QMessageBox::StandardButton reply =
                QMessageBox::question(this, tr("广播图片"),
                                      tr("确定要将图片 %1 广播给所有 %2 个客户端吗?")
                                          .arg(fileInfo.fileName())
                                          .arg(server->clientCount()),
                                      QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes)
            {
                if (server->sendImage(imagePath))
                {
                    appendTimestampedMessage(tr("广播图片给 %1 个客户端: %2")
                                                 .arg(server->clientCount())
                                                 .arg(fileInfo.fileName()),
                                             SentMessage);

                    // 在消息区域显示图片预览
                    QImage image(imagePath);
                    if (!image.isNull())
                    {
                        QImage scaledImage = image.scaledToWidth(300, Qt::SmoothTransformation);
                        QByteArray byteArray;
                        QBuffer buffer(&byteArray);
                        buffer.open(QIODevice::WriteOnly);
                        scaledImage.save(&buffer, "PNG");
                        QString html = QString("<img src='data:image/png;base64,%1' />")
                                           .arg(QString(byteArray.toBase64()));
                        ui->logTextEdit->append(html);
                    }
                }
            }
        }
        else
        {
            // 发送给特定客户端
            QString clientInfo = ui->targetClientComboBox->currentText();
            if (server->sendImageToClient(clientInfo, imagePath))
            {
                appendTimestampedMessage(
                    tr("发送图片给 %1: %2").arg(clientInfo).arg(fileInfo.fileName()), SentMessage);

                // 在消息区域显示图片预览
                QImage image(imagePath);
                if (!image.isNull())
                {
                    QImage scaledImage = image.scaledToWidth(300, Qt::SmoothTransformation);
                    QByteArray byteArray;
                    QBuffer buffer(&byteArray);
                    buffer.open(QIODevice::WriteOnly);
                    scaledImage.save(&buffer, "PNG");
                    QString html = QString("<img src='data:image/png;base64,%1' />")
                                       .arg(QString(byteArray.toBase64()));
                    ui->logTextEdit->append(html);
                }
            }
        }
    }
}

void MainWindow::onClientFileReceived(const QString &fileName, qint64 fileSize,
                                      const QString &fileType, const QByteArray &fileData)
{
    appendTimestampedMessage(tr("收到文件: %1 (%2 字节)").arg(fileName).arg(fileSize),
                             ReceivedMessage);

    // 询问用户是否保存文件
    QString saveDir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"));
    if (!saveDir.isEmpty())
    {
        QString savePath = QDir(saveDir).filePath(fileName);
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(fileData);
            file.close();
            appendToLog(tr("文件已保存至: %1").arg(savePath));
        }
        else
        {
            appendToLog(tr("无法保存文件: %1").arg(file.errorString()));
        }
    }
}

void MainWindow::onClientImageReceived(const QString &imageName, qint64 imageSize,
                                       const QString &imageType, const QByteArray &imageData)
{
    appendTimestampedMessage(tr("收到图片: %1 (%2 字节)").arg(imageName).arg(imageSize),
                             ReceivedMessage);

    // 直接在消息区域显示图片
    QImage image;
    image.loadFromData(imageData);
    if (!image.isNull())
    {
        // 调整图片大小以适应消息区域
        QImage scaledImage = image.scaledToWidth(300, Qt::SmoothTransformation);
        QByteArray displayData;
        QBuffer buffer(&displayData);
        buffer.open(QIODevice::WriteOnly);
        scaledImage.save(&buffer, "PNG");
        QString html =
            QString("<img src='data:image/png;base64,%1' />").arg(QString(displayData.toBase64()));
        ui->logTextEdit->append(html);

        // 询问用户是否保存图片
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, tr("保存图片"), tr("是否保存图片 %1?").arg(imageName),
                                  QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            QString saveDir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"));
            if (!saveDir.isEmpty())
            {
                QString savePath = QDir(saveDir).filePath(imageName);
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly))
                {
                    file.write(imageData);
                    file.close();
                    appendToLog(tr("图片已保存至: %1").arg(savePath));
                }
                else
                {
                    appendToLog(tr("无法保存图片: %1").arg(file.errorString()));
                }
            }
        }
    }
    else
    {
        appendToLog(tr("无法显示接收到的图片"));
    }
}

void MainWindow::onServerFileReceived(const QString &clientInfo, const QString &fileName,
                                      qint64 fileSize, const QString &fileType,
                                      const QByteArray &fileData)
{
    appendTimestampedMessage(
        tr("收到来自 %1 的文件: %2 (%3 字节)").arg(clientInfo).arg(fileName).arg(fileSize),
        ReceivedMessage);

    // 询问用户是否保存文件
    QString saveDir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"));
    if (!saveDir.isEmpty())
    {
        QString savePath = QDir(saveDir).filePath(fileName);
        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(fileData);
            file.close();
            appendToLog(tr("文件已保存至: %1").arg(savePath));
        }
        else
        {
            appendToLog(tr("无法保存文件: %1").arg(file.errorString()));
        }
    }
}

void MainWindow::onServerImageReceived(const QString &clientInfo, const QString &imageName,
                                       qint64 imageSize, const QString &imageType,
                                       const QByteArray &imageData)
{
    appendTimestampedMessage(
        tr("收到来自 %1 的图片: %2 (%3 字节)").arg(clientInfo).arg(imageName).arg(imageSize),
        ReceivedMessage);

    // 直接在消息区域显示图片
    QImage image;
    image.loadFromData(imageData);
    if (!image.isNull())
    {
        // 调整图片大小以适应消息区域
        QImage scaledImage = image.scaledToWidth(300, Qt::SmoothTransformation);
        QByteArray displayData;
        QBuffer buffer(&displayData);
        buffer.open(QIODevice::WriteOnly);
        scaledImage.save(&buffer, "PNG");
        QString html =
            QString("<img src='data:image/png;base64,%1' />").arg(QString(displayData.toBase64()));
        ui->logTextEdit->append(html);

        // 询问用户是否保存图片
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("保存图片"), tr("是否保存来自 %1 的图片 %2?").arg(clientInfo).arg(imageName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            QString saveDir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"));
            if (!saveDir.isEmpty())
            {
                QString savePath = QDir(saveDir).filePath(imageName);
                QFile file(savePath);
                if (file.open(QIODevice::WriteOnly))
                {
                    file.write(imageData);
                    file.close();
                    appendToLog(tr("图片已保存至: %1").arg(savePath));
                }
                else
                {
                    appendToLog(tr("无法保存图片: %1").arg(file.errorString()));
                }
            }
        }
    }
    else
    {
        appendToLog(tr("无法显示接收到的图片"));
    }
}