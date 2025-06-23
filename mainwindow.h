#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QList>

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
    
    // 客户端相关槽函数
    void onClientSocketConnected();
    void onClientSocketDisconnected();
    void onClientSocketReadyRead();
    void onClientSocketError(QAbstractSocket::SocketError socketError);
    
    // 服务端相关槽函数
    void onServerNewConnection();
    void onServerClientDisconnected();
    void onServerClientReadyRead();

private:
    Ui::MainWindow *ui;
    
    // 客户端相关
    QTcpSocket *clientSocket;
    
    // 服务端相关
    QTcpServer *server;
    QList<QTcpSocket*> serverClients;
    
    // 当前模式
    enum Mode {
        ClientMode,
        ServerMode
    } currentMode;
    
    void updateUI();
    void appendToLog(const QString &message);
    void sendMessage();
    void setupConnections();
};

#endif // MAINWINDOW_H 