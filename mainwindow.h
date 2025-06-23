#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "TCPClient.h"
#include "TCPServer.h"

namespace Ui {
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

private:
    Ui::MainWindow *ui;
    
    // 客户端和服务端
    TCPClient *client;
    TCPServer *server;
    
    // 当前模式
    enum Mode {
        ClientMode,
        ServerMode
    } currentMode;
    
    void updateUI();
    void appendToLog(const QString &message);
    void sendMessage();
    void setupConnections();
    
    // 更新编码设置
    void updateEncodingSettings();
};

#endif // MAINWINDOW_H 