#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "TCPClient.h"
#include "TCPServer.h"
#include <QComboBox>
#include <QDateTime>
#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private slots:
    void on_modeComboBox_currentTextChanged(const QString &mode);
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_connectButton_clicked();
    void on_sendButton_clicked();
    void on_messageEdit_returnPressed();
    void on_actionNewWindow_triggered();
    void on_sendEncodingComboBox_currentIndexChanged(int index);
    void on_receiveEncodingComboBox_currentIndexChanged(int index);
    void on_targetClientComboBox_currentIndexChanged(int index);

    // 客户端相关槽函数
    void onClientConnected();
    void onClientDisconnected();
    void onClientMessageReceived(const QString &message);
    void onClientError(const QString &errorMessage);

    // 服务端相关槽函数
    void onServerStarted(int port);
    void onServerStopped();
    void onServerClientConnected(const QString &clientInfo);
    void onServerClientDisconnected(const QString &clientInfo);
    void onServerMessageReceived(const QString &clientInfo, const QString &message);
    void onServerError(const QString &errorMessage);

    // 文件传输相关槽函数
    void on_sendFileButton_clicked();
    void on_sendImageButton_clicked();
    void onClientFileReceived(const QString &fileName, qint64 fileSize, const QString &fileType,
                              const QByteArray &fileData);
    void onClientImageReceived(const QString &imageName, qint64 imageSize, const QString &imageType,
                               const QByteArray &imageData);
    void onServerFileReceived(const QString &clientInfo, const QString &fileName, qint64 fileSize,
                              const QString &fileType, const QByteArray &fileData);
    void onServerImageReceived(const QString &clientInfo, const QString &imageName,
                               qint64 imageSize, const QString &imageType,
                               const QByteArray &imageData);

  private:
    Ui::MainWindow *ui;

    // 客户端和服务端
    TCPClient *client;
    TCPServer *server;

    // 客户端选择下拉框
    QComboBox *targetClientComboBox;

    // 当前模式
    enum Mode
    {
        ClientMode,
        ServerMode
    } currentMode;

    // 消息类型
    enum MessageType
    {
        SystemMessage,   // 系统消息 - 灰色
        SentMessage,     // 发送的消息 - 黑色
        ReceivedMessage, // 接收的消息 - 黑色
        TimeStamp        // 时间戳 - 蓝色
    };

    void updateUI();
    void appendToLog(const QString &message, MessageType type = SystemMessage);
    void appendTimestampedMessage(const QString &message, MessageType type);
    void sendMessage();
    void setupConnections();

    // 更新编码设置
    void updateEncodingSettings();

    // 更新客户端列表
    void updateClientList();
};

#endif // MAINWINDOW_H