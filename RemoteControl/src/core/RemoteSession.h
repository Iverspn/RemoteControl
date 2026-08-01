#ifndef REMOTESESSION_H
#define REMOTESESSION_H

#include <QObject>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QPoint>
#include "model/Protocol.h"

class ScreenCapture;

class RemoteSession : public QObject
{
    Q_OBJECT
public:
    explicit RemoteSession(QObject *parent = nullptr);
    ~RemoteSession();

    // ── 主机模块 ──
    void startHost(quint16 port, const QString &password, int quality, int fps);
    void stopHost();
    void regeneratePassword();

    // ── 客户端模块 ──
    void connectToHost(const QString &host, quint16 port,
                       const QString &deviceId, const QString &password);
    void disconnectFromHost();

    // ── 设置 ──
    void setQuality(int quality);
    void setFps(int fps);

    // ── 输入转发（从客户端的界面调用） ──
    void sendMouseMove(int x, int y);
    void sendMousePress(const QString &button);
    void sendMouseRelease(const QString &button);
    void sendMouseEvent(const QString &button, bool isPress, int x, int y);
    void sendMouseWheel(int delta);
    void sendKeyPress(int vkCode);
    void sendKeyRelease(int vkCode);

    // ── 文件传输 ──
    void sendFile(const QString &localPath);
    void requestFile(const QString &remotePath, const QString &localSavePath);

    // ── 状态器 ──
    QString deviceId() const;
    QString currentPassword() const;
    bool    isHosting() const;
    bool    isConnected() const;

signals:
    // 主机生命周期
    void hostStarted();
    void hostStopped();
    void clientConnected(const QString &clientIp);
    void clientDisconnected();

    // 客户端生命周期
    void connectedToHost();
    void disconnectedFromHost();
    void authSuccess();
    void authFailed(const QString &reason);

    // 数据
    void frameReceived(const QByteArray &jpegData, int width, int height);

    // 文件 transfer
    void fileReceiveProgress(const QString &fileName, qint64 received, qint64 total);
    void fileReceiveComplete(const QString &fileName);

    // Errors
    void connectionError(const QString &message);

private slots:
    // 主机
    void onNewConnection();
    void onHostSocketDisconnected();
    void onHostReadyRead();

    // 客户端
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onClientReadyRead();

    // 屏幕捕获
    void onFrameCaptured(const QByteArray &jpegData, int width, int height);
    void onReconnectTimer();

private:
    // ── 线路协议（低级别） ──
    void sendMessage(QTcpSocket *socket, const QJsonObject &msg);
    void sendFrame(QTcpSocket *socket, const QByteArray &jpegData, int width, int height);
    void sendFilePacket(QTcpSocket *socket, const QString &fileName,
                        const QByteArray &data, qint64 totalSize);

    // 从套接字的读取缓冲区读取，每次处理一条完整消息
    void processBuffer(QTcpSocket *socket, QByteArray &readBuffer);
    void handleJsonMessage(QTcpSocket *socket, const QJsonObject &msg);
    void handleFilePacket(QTcpSocket *socket, QDataStream &stream, quint32 payloadLen);

    // ── 认证 ──
    QString generateDeviceId() const;
    QString generatePassword() const;

    // ── 状态 ──
    enum class State {
        Idle,                   //空闲
        Host_Listening,         //主机监听中
        Host_Connected,         //主机已连接
        Client_Connecting,      //客户端连接中
        Client_Authenticating,  //客户端认证中
        Client_Connected        //客户端已连接
    };
    State m_state = State::Idle;

    // ── 主机模块成员 ──
    QTcpServer *m_server       = nullptr;
    QTcpSocket *m_hostSocket   = nullptr;  // 连接客户端socket
    QByteArray  m_hostBuffer;

    // ── 客户端模块成员 ──
    QTcpSocket *m_clientSocket = nullptr;
    QByteArray  m_clientBuffer;
    QString     m_lastHost;
    quint16     m_lastPort     = 0;
    QString     m_lastDeviceId;
    QString     m_lastPassword;

    // ── 通用数据成员 ──
    ScreenCapture *m_capture         = nullptr;
    QTimer        *m_reconnectTimer  = nullptr;
    QTimer        *m_heartbeatTimer  = nullptr;
    QTimer        *m_pongTimer       = nullptr;
    QTimer        *m_activityTimer   = nullptr;
    QTimer        *m_inputFlushTimer = nullptr;
    QPoint         m_pendingMousePos;
    bool           m_hasPendingMouse = false;
    bool           m_heartbeatPending = false;
    QString        m_password;
    QString        m_deviceId;
    int            m_quality       = DEFAULT_QUALITY;
    int            m_fps           = DEFAULT_FPS;
    bool           m_manualDisconnect = false;

    // ── 文件接受状态 ──
    struct FileReceiveState {
        bool    active   = false;
        QString fileName;
        qint64  fileSize = 0;
        qint64  received = 0;
        QByteArray dataAccum;
    };
    FileReceiveState m_fileRx;
};

#endif // REMOTESESSION_H
