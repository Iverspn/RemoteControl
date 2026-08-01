#include "RemoteSession.h"
#include "ScreenCapture.h"
#include "InputInjector.h"
#include "model/Protocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSysInfo>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QHostInfo>
#include <QNetworkInterface>

// ═══════════════════════════════════════════════════════════════
// Construction / Destruction
// ═══════════════════════════════════════════════════════════════

RemoteSession::RemoteSession(QObject *parent)
    : QObject(parent)
{
    m_deviceId = generateDeviceId();
    m_password = generatePassword();

    m_capture = new ScreenCapture(this);
    connect(m_capture, &ScreenCapture::frameCaptured,
            this, &RemoteSession::onFrameCaptured);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &RemoteSession::onReconnectTimer);

    // 心跳：每 30 秒发 ping，120 秒无响应断开（LAN 足够）
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        m_heartbeatPending = true;
        QJsonObject ping;
        ping["action"] = "ping";
        ping["data"] = QJsonObject{};
        // 重置 pong 超时（120 秒）
        m_pongTimer->start(120000);
        sendMessage(m_hostSocket, ping);
        sendMessage(m_clientSocket, ping);
    });

    m_pongTimer = new QTimer(this);
    m_pongTimer->setSingleShot(true);

    // 收到数据时重置超时（帧传输 = 连接存活）
    m_activityTimer = new QTimer(this);
    m_activityTimer->setSingleShot(true);
    m_activityTimer->setInterval(120000);
    connect(m_activityTimer, &QTimer::timeout, this, [this]() {
        emit connectionError("连接已断开（120秒无数据）");
        if (m_hostSocket) m_hostSocket->disconnectFromHost();
        if (m_clientSocket) m_clientSocket->disconnectFromHost();
    });

    // 输入合并定时器：每 2ms 发送一次，避免高频鼠标移动产生大量小包
    m_inputFlushTimer = new QTimer(this);
    m_inputFlushTimer->setInterval(2);
    connect(m_inputFlushTimer, &QTimer::timeout, this, [this]() {
        if (m_hasPendingMouse && m_clientSocket
            && m_clientSocket->state() == QAbstractSocket::ConnectedState) {
            QJsonObject msg;
            msg["action"] = "input_mouse_move";
            QJsonObject posData;
            posData["x"] = m_pendingMousePos.x();
            posData["y"] = m_pendingMousePos.y();
            msg["data"] = posData;
            sendMessage(m_clientSocket, msg);
            m_hasPendingMouse = false;
        }
        m_inputFlushTimer->stop();
    });
}

RemoteSession::~RemoteSession()
{
    stopHost();
    disconnectFromHost();
}

// ═══════════════════════════════════════════════════════════════
// Host mode
// ═══════════════════════════════════════════════════════════════

void RemoteSession::startHost(quint16 port, const QString &password,
                               int quality, int fps)
{
    if (m_state != State::Idle) return;

    m_password = password;
    m_quality = quality;
    m_fps = fps;

    m_capture->setQuality(quality);
    m_capture->setFps(fps);

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &RemoteSession::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit connectionError("监听失败: " + m_server->errorString());
        delete m_server;
        m_server = nullptr;
        return;
    }

    m_state = State::Host_Listening;
    emit hostStarted();
}

void RemoteSession::stopHost()
{
    m_heartbeatTimer->stop();
    m_pongTimer->stop();
    m_activityTimer->stop();

    m_state = State::Idle;

    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }

    if (m_hostSocket) {
        QTcpSocket *sock = m_hostSocket;
        m_hostSocket = nullptr;
        sock->disconnectFromHost();
        sock->deleteLater();
    }

    m_capture->stop();
    m_hostBuffer.clear();
    emit hostStopped();
}

void RemoteSession::regeneratePassword()
{
    m_password = generatePassword();
}

void RemoteSession::onNewConnection()
{
    if (!m_server) return;

    // 在 v1 中只接受一个客户端
    if (m_state == State::Host_Connected) {
        // 拒绝其他连接
        QTcpSocket *rejected = m_server->nextPendingConnection();//取出新连接的socket
        if (rejected) {
            //构造json的错误消息
            QJsonObject err;
            err["type"] = "error";
            err["data"] = QJsonObject{{"message", "已有其他客户端连接"}};

            sendMessage(rejected, err);
            rejected->disconnectFromHost();//断开连接
            rejected->deleteLater();//删除
        }
        return;
    }

    m_hostSocket = m_server->nextPendingConnection();
    if (!m_hostSocket) return;

    m_hostSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_hostSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 256 * 1024);
    m_hostSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 256 * 1024);
    m_hostBuffer.clear();

    connect(m_hostSocket, &QTcpSocket::readyRead,
            this, &RemoteSession::onHostReadyRead);
    connect(m_hostSocket, &QTcpSocket::disconnected,
            this, &RemoteSession::onHostSocketDisconnected);
}

void RemoteSession::onHostSocketDisconnected()
{
    if (!m_hostSocket) return;

    m_hostSocket->deleteLater();
    m_hostSocket = nullptr;
    m_capture->stop();
    m_hostBuffer.clear();
    m_heartbeatTimer->stop();
    m_pongTimer->stop();
    m_activityTimer->stop();
    m_heartbeatPending = false;
    if (m_state == State::Host_Connected) {
        m_state = State::Host_Listening;
    }
    emit clientDisconnected();
}

void RemoteSession::onHostReadyRead()
{
    processBuffer(m_hostSocket, m_hostBuffer);
}

// ═══════════════════════════════════════════════════════════════
// Client mode
// ═══════════════════════════════════════════════════════════════

void RemoteSession::connectToHost(const QString &host, quint16 port,
                                   const QString &deviceId,
                                   const QString &password)
{
    if (m_state != State::Idle) return;

    m_reconnectTimer->stop();

    m_lastHost = host;
    m_lastPort = port;
    m_lastDeviceId = deviceId;
    m_lastPassword = password;
    m_password = password;
    m_manualDisconnect = false;

    m_clientSocket = new QTcpSocket(this);
    m_clientSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_clientSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 256 * 1024);
    m_clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 256 * 1024);
    connect(m_clientSocket, &QTcpSocket::connected,
            this, &RemoteSession::onSocketConnected);
    connect(m_clientSocket, &QTcpSocket::disconnected,
            this, &RemoteSession::onSocketDisconnected);
    connect(m_clientSocket, &QTcpSocket::errorOccurred,
            this, &RemoteSession::onSocketError);
    connect(m_clientSocket, &QTcpSocket::readyRead,
            this, &RemoteSession::onClientReadyRead);

    m_clientBuffer.clear();
    m_state = State::Client_Connecting;

    // 连接超时：5 秒没连上就报错
    QTimer *timeout = new QTimer(this);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, this, [this, timeout]() {
        if (m_state == State::Client_Connecting && m_clientSocket) {
            m_clientSocket->abort();
            emit connectionError("连接超时：无法连接到 " + m_lastHost + ":" + QString::number(m_lastPort)
                               + "\n请确认：① 被控端已启动监听 ② IP和端口正确 ③ 防火墙已放行");
        }
        timeout->deleteLater();
    });
    timeout->start(5000);

    m_clientSocket->connectToHost(host, port);
}

void RemoteSession::disconnectFromHost()
{
    m_manualDisconnect = true;
    m_reconnectTimer->stop();
    m_heartbeatTimer->stop();
    m_pongTimer->stop();
    m_activityTimer->stop();
    m_heartbeatPending = false;

    if (m_clientSocket) {
        QTcpSocket *sock = m_clientSocket;
        m_clientSocket = nullptr;
        sock->disconnectFromHost();
        sock->deleteLater();
    }

    m_clientBuffer.clear();
    m_state = State::Idle;
    emit disconnectedFromHost();
}

void RemoteSession::onSocketConnected()
{
    m_reconnectTimer->stop();
    m_state = State::Client_Authenticating;

    // Send auth
    QJsonObject auth;
    auth["action"] = "auth";
    auth["data"] = QJsonObject{
        {"deviceId", m_lastDeviceId},
        {"password", m_lastPassword}
    };
    sendMessage(m_clientSocket, auth);
}

void RemoteSession::onSocketDisconnected()
{
    if (!m_clientSocket) return;

    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;
    m_clientBuffer.clear();
    m_state = State::Idle;
    emit disconnectedFromHost();

    // Auto-reconnect
    if (!m_manualDisconnect && !m_lastHost.isEmpty()) {
        m_reconnectTimer->start(3000);
    }
}

void RemoteSession::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (m_clientSocket) {
        QString msg = m_clientSocket->errorString();
        if (error == QAbstractSocket::HostNotFoundError)
            msg = "无法解析主机名: " + m_lastHost;
        else if (error == QAbstractSocket::ConnectionRefusedError)
            msg = "连接被拒绝: " + m_lastHost + ":" + QString::number(m_lastPort)
                + "\n请确认被控端已启动监听且防火墙已放行端口";
        else if (error == QAbstractSocket::NetworkError)
            msg = "网络不可达，请检查IP和网络连接";
        emit connectionError(msg);
    }
}

void RemoteSession::onClientReadyRead()
{
    processBuffer(m_clientSocket, m_clientBuffer);
}

void RemoteSession::onReconnectTimer()
{
    if (m_manualDisconnect || m_lastHost.isEmpty()) return;

    if (m_clientSocket) {
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    m_clientSocket = new QTcpSocket(this);
    m_clientSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_clientSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 256 * 1024);
    m_clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 256 * 1024);
    connect(m_clientSocket, &QTcpSocket::connected,
            this, &RemoteSession::onSocketConnected);
    connect(m_clientSocket, &QTcpSocket::disconnected,
            this, &RemoteSession::onSocketDisconnected);
    connect(m_clientSocket, &QTcpSocket::errorOccurred,
            this, &RemoteSession::onSocketError);
    connect(m_clientSocket, &QTcpSocket::readyRead,
            this, &RemoteSession::onClientReadyRead);

    m_clientBuffer.clear();
    m_state = State::Client_Connecting;
    m_clientSocket->connectToHost(m_lastHost, m_lastPort);
}

// ═══════════════════════════════════════════════════════════════
// Settings
// ═══════════════════════════════════════════════════════════════

void RemoteSession::setQuality(int quality)
{
    m_quality = qBound(10, quality, 100);
    m_capture->setQuality(m_quality);

    // Notify host side if in client mode
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "set_quality";
        msg["data"] = QJsonObject{{"quality", m_quality}};
        sendMessage(m_clientSocket, msg);
    }
}

void RemoteSession::setFps(int fps)
{
    m_fps = qBound(1, fps, 30);
    m_capture->setFps(m_fps);

    // Notify host side if in client mode
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "set_fps";
        msg["data"] = QJsonObject{{"fps", m_fps}};
        sendMessage(m_clientSocket, msg);
    }
}

// ═══════════════════════════════════════════════════════════════
// Input forwarding
// ═══════════════════════════════════════════════════════════════

void RemoteSession::sendMouseMove(int x, int y)
{
    // 合并高频鼠标移动：只记录最新位置，由 inputFlushTimer 定时发送
    m_pendingMousePos = QPoint(x, y);
    m_hasPendingMouse = true;
    if (!m_inputFlushTimer->isActive())
        m_inputFlushTimer->start();
}

void RemoteSession::sendMouseEvent(const QString &button, bool isPress, int x, int y)
{
    // 合并鼠标点击+位置为单条消息
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "input_mouse_event";
        msg["data"] = QJsonObject{{"button", button}, {"isPress", isPress},
                                  {"x", x}, {"y", y}};
        sendMessage(m_clientSocket, msg);
    }
}

void RemoteSession::sendMousePress(const QString &button)
{
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "input_mouse_press";
        msg["data"] = QJsonObject{{"button", button}};
        sendMessage(m_clientSocket, msg);
    }
}

void RemoteSession::sendMouseRelease(const QString &button)
{
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "input_mouse_release";
        msg["data"] = QJsonObject{{"button", button}};
        sendMessage(m_clientSocket, msg);
    }
}

void RemoteSession::sendMouseWheel(int delta)
{
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "input_mouse_wheel";
        msg["data"] = QJsonObject{{"delta", delta}};
        sendMessage(m_clientSocket, msg);
    }
}

void RemoteSession::sendKeyPress(int vkCode)
{
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "input_key_press";
        msg["data"] = QJsonObject{{"vkCode", vkCode}};
        sendMessage(m_clientSocket, msg);
    }
}

void RemoteSession::sendKeyRelease(int vkCode)
{
    if (m_state == State::Client_Connected && m_clientSocket) {
        QJsonObject msg;
        msg["action"] = "input_key_release";
        msg["data"] = QJsonObject{{"vkCode", vkCode}};
        sendMessage(m_clientSocket, msg);
    }
}

// ═══════════════════════════════════════════════════════════════
// File transfer
// ═══════════════════════════════════════════════════════════════

void RemoteSession::sendFile(const QString &localPath)
{
    QTcpSocket *socket = nullptr;
    if (m_state == State::Client_Connected && m_clientSocket) {
        socket = m_clientSocket;
    } else if (m_state == State::Host_Connected && m_hostSocket) {
        socket = m_hostSocket;
    }
    if (!socket) return;

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit connectionError("无法打开文件: " + localPath);
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QFileInfo fi(localPath);
    sendFilePacket(socket, fi.fileName(), fileData, fileData.size());
}

void RemoteSession::requestFile(const QString &remotePath,
                                 const QString &localSavePath)
{
    Q_UNUSED(remotePath)
    // For v1: request is stored locally; host already sent or we'll handle
    // Store the save path for when file data arrives
    m_fileRx.fileName = localSavePath;
}

// ═══════════════════════════════════════════════════════════════
// Wire protocol (low-level)
// ═══════════════════════════════════════════════════════════════

void RemoteSession::sendMessage(QTcpSocket *socket, const QJsonObject &msg)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray json = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    // 帧头 + JSON 一次分配，一次写入（控制消息小，拷贝开销可忽略）
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << MAGIC_JSON << static_cast<quint32>(json.size());
    packet.append(json);

    socket->write(packet);
}

void RemoteSession::sendFrame(QTcpSocket *socket, const QByteArray &jpegData,
                               int width, int height)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    quint32 framePayloadLen = static_cast<quint32>(8 + jpegData.size());

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << MAGIC_FRAME << framePayloadLen
           << static_cast<quint32>(width) << static_cast<quint32>(height);
    packet.append(jpegData);

    socket->write(packet);
}

void RemoteSession::sendFilePacket(QTcpSocket *socket, const QString &fileName,
                                    const QByteArray &data, qint64 totalSize)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray nameBytes = fileName.toUtf8();
    quint32 payloadLen = static_cast<quint32>(4 + 8 + nameBytes.size() + data.size());

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << MAGIC_FILE << payloadLen
           << static_cast<quint32>(nameBytes.size())
           << static_cast<qint64>(totalSize);
    packet.append(nameBytes);
    packet.append(data);

    socket->write(packet);
}

// ═══════════════════════════════════════════════════════════════
// Buffer processing (TCP reassembly state machine)
// ═══════════════════════════════════════════════════════════════

void RemoteSession::processBuffer(QTcpSocket *socket, QByteArray &readBuffer)
{
    constexpr int MAX_BUFFER_SIZE = 128 * 1024 * 1024; // 128MB（支持 30fps）

    readBuffer.append(socket->readAll());
    // 收到数据 = 连接存活
    m_activityTimer->start();

    // 缓冲区超限保护：丢弃脏数据，断开连接
    if (readBuffer.size() > MAX_BUFFER_SIZE) {
        readBuffer.clear();
        socket->disconnectFromHost();
        return;
    }

    while (readBuffer.size() >= 8) {
        QDataStream stream(readBuffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 magic = 0;
        quint32 length = 0;
        stream >> magic >> length;

        if (magic != MAGIC_JSON && magic != MAGIC_FRAME && magic != MAGIC_FILE) {
            // 用大端序构造搜索模式（修复端序 BUG）
            auto makeBePattern = [](quint32 m) -> QByteArray {
                QByteArray be;
                QDataStream ms(&be, QIODevice::WriteOnly);
                ms.setByteOrder(QDataStream::BigEndian);
                ms << m;
                return be;
            };
            int nextIdx = readBuffer.indexOf(makeBePattern(MAGIC_JSON), 1);
            if (nextIdx == -1)
                nextIdx = readBuffer.indexOf(makeBePattern(MAGIC_FRAME), 1);
            if (nextIdx == -1)
                nextIdx = readBuffer.indexOf(makeBePattern(MAGIC_FILE), 1);
            if (nextIdx == -1) {
                readBuffer.clear();
                return;
            }
            readBuffer.remove(0, nextIdx);
            continue;
        }

        int totalNeeded = 8 + static_cast<int>(length);
        if (readBuffer.size() < totalNeeded) {
            return;
        }

        QByteArray payload = readBuffer.mid(8, static_cast<int>(length));
        readBuffer.remove(0, totalNeeded);

        if (magic == MAGIC_JSON) {
            QJsonDocument doc = QJsonDocument::fromJson(payload);
            if (!doc.isNull() && doc.isObject()) {
                handleJsonMessage(socket, doc.object());
            }
        } else if (magic == MAGIC_FRAME) {
            if (payload.size() >= 8) {
                QDataStream frameStream(&payload, QIODevice::ReadOnly);
                frameStream.setByteOrder(QDataStream::BigEndian);
                quint32 w = 0, h = 0;
                frameStream >> w >> h;
                QByteArray jpeg = payload.mid(8);
                emit frameReceived(jpeg, static_cast<int>(w), static_cast<int>(h));
            }
        } else if (magic == MAGIC_FILE) {
            QDataStream fileStream(payload);
            fileStream.setByteOrder(QDataStream::BigEndian);
            handleFilePacket(socket, fileStream, length);
        }
    }
}

void RemoteSession::handleJsonMessage(QTcpSocket *socket, const QJsonObject &msg)
{
    QString action = msg.value("action").toString();
    QJsonObject data = msg.value("data").toObject();

    // ── Determine which socket role we're handling ──
    bool isHost = (socket == m_hostSocket);

    if (action == "auth") {
        // Only host handles auth
        if (!isHost) return;

        QString deviceId = data.value("deviceId").toString();
        QString password = data.value("password").toString();

        if (password == m_password) {
            Q_UNUSED(deviceId)
            // Auth success
            QJsonObject response;
            response["type"] = "auth_success";
            response["data"] = QJsonObject{};
            sendMessage(m_hostSocket, response);

            m_state = State::Host_Connected;
            QString clientIp = m_hostSocket->peerAddress().toString();
            emit clientConnected(clientIp);

            // 启动心跳和画面采集
            m_heartbeatPending = false;
            m_heartbeatTimer->start();
            m_activityTimer->start();

            m_capture->setQuality(m_quality);
            m_capture->setFps(m_fps);
            m_capture->start();
        } else {
            QJsonObject response;
            response["type"] = "auth_failed";
            response["data"] = QJsonObject{{"reason", "密码错误"}};
            sendMessage(m_hostSocket, response);

            // Close connection on auth failure
            m_hostSocket->disconnectFromHost();
        }
        return;
    }

    // ── Messages from host to client ──
    if (!isHost && socket == m_clientSocket) {
        QString type = msg.value("type").toString();
        if (type == "auth_success") {
            m_state = State::Client_Connected;
            m_heartbeatPending = false;
            m_heartbeatTimer->start();
            m_activityTimer->start();
            emit connectedToHost();
            emit authSuccess();
            return;
        }
        if (type == "auth_failed") {
            QString reason = data.value("reason").toString("未知原因");
            emit authFailed(reason);
            disconnectFromHost();
            return;
        }
        if (type == "error") {
            emit connectionError(data.value("message").toString());
            return;
        }
        if (type == "ping") {
            QJsonObject pong;
            pong["type"] = "pong";
            pong["data"] = QJsonObject{};
            sendMessage(m_clientSocket, pong);
            return;
        }
        if (type == "pong") {
            m_heartbeatPending = false;
            m_pongTimer->start();
            return;
        }
        return;
    }

    // ── Messages from client to host (input forwarding, settings, heartbeat) ──
    if (isHost && m_state == State::Host_Connected) {
        if (action == "ping") {
            QJsonObject pong;
            pong["type"] = "pong";
            pong["data"] = QJsonObject{};
            sendMessage(m_hostSocket, pong);
            return;
        }
        if (action == "pong") {
            m_heartbeatPending = false;
            m_pongTimer->start();
            return;
        }
        if (action == "set_quality") {
            m_quality = data.value("quality").toInt(50);
            m_capture->setQuality(m_quality);
        } else if (action == "set_fps") {
            m_fps = data.value("fps").toInt(10);
            m_capture->setFps(m_fps);
        } else if (action == "input_mouse_move") {
            qreal s = m_capture->captureScale();
            InputInjector::mouseMove(qRound(data.value("x").toInt() / s),
                                      qRound(data.value("y").toInt() / s));
        } else if (action == "input_mouse_event") {
            // 合并鼠标位置+点击：单条消息，减少网络往返
            qreal s = m_capture->captureScale();
            int x = qRound(data.value("x").toInt() / s);
            int y = qRound(data.value("y").toInt() / s);
            QString btn = data.value("button").toString();
            bool isPress = data.value("isPress").toBool();
            InputInjector::mouseMove(x, y);
            if (isPress) {
                if (btn == "left") InputInjector::mousePress(Qt::LeftButton);
                else if (btn == "right") InputInjector::mousePress(Qt::RightButton);
                else if (btn == "middle") InputInjector::mousePress(Qt::MiddleButton);
            } else {
                if (btn == "left") InputInjector::mouseRelease(Qt::LeftButton);
                else if (btn == "right") InputInjector::mouseRelease(Qt::RightButton);
                else if (btn == "middle") InputInjector::mouseRelease(Qt::MiddleButton);
            }
        } else if (action == "input_mouse_press") {
            QString btn = data.value("button").toString();
            if (btn == "left") InputInjector::mousePress(Qt::LeftButton);
            else if (btn == "right") InputInjector::mousePress(Qt::RightButton);
            else if (btn == "middle") InputInjector::mousePress(Qt::MiddleButton);
        } else if (action == "input_mouse_release") {
            QString btn = data.value("button").toString();
            if (btn == "left") InputInjector::mouseRelease(Qt::LeftButton);
            else if (btn == "right") InputInjector::mouseRelease(Qt::RightButton);
            else if (btn == "middle") InputInjector::mouseRelease(Qt::MiddleButton);
        } else if (action == "input_mouse_wheel") {
            InputInjector::mouseWheel(data.value("delta").toInt());
        } else if (action == "input_key_press") {
            InputInjector::keyPress(data.value("vkCode").toInt());
        } else if (action == "input_key_release") {
            InputInjector::keyRelease(data.value("vkCode").toInt());
        } else if (action == "file_send_request") {
            // Client wants to send a file to host
            // File data follows as MAGIC_FILE packet
        } else if (action == "file_receive_request") {
            // Client wants to receive a file from host
            QString remotePath = data.value("remotePath").toString();
            Q_UNUSED(remotePath)
            // For v1: host sends a test file or selected file
            // Notify client that file sending starts
            QJsonObject notify;
            notify["type"] = "file_send_start";
            notify["data"] = QJsonObject{
                {"fileName", "remote_file.bin"},
                {"fileSize", 0}
            };
            sendMessage(m_hostSocket, notify);
        }
    }
}

void RemoteSession::handleFilePacket(QTcpSocket *socket, QDataStream &stream,
                                      quint32 payloadLen)
{
    Q_UNUSED(socket)

    quint32 nameLen = 0;
    qint64 fileSize = 0;
    stream >> nameLen >> fileSize;

    // 文件名长度安全校验
    constexpr quint32 MAX_NAME_LEN = 260;
    if (nameLen > MAX_NAME_LEN || nameLen == 0) return;

    // 数据长度安全校验（防止负数分配）
    int remaining = static_cast<int>(4 + 8 + nameLen);
    if (remaining >= static_cast<int>(payloadLen)) return; // 恶意包
    int dataLen = static_cast<int>(payloadLen) - remaining;

    QByteArray nameBytes(static_cast<int>(nameLen), Qt::Uninitialized);
    stream.readRawData(nameBytes.data(), static_cast<int>(nameLen));
    QString fileName = QString::fromUtf8(nameBytes);

    QByteArray fileData(dataLen, Qt::Uninitialized);
    stream.readRawData(fileData.data(), dataLen);

    QString savePath = m_fileRx.fileName;
    if (savePath.isEmpty()) {
        savePath = QDir::homePath() + "/Desktop/" + fileName;
    }

    QFile outFile(savePath);
    if (outFile.open(QIODevice::WriteOnly)) {
        outFile.write(fileData);
        outFile.close();
        emit fileReceiveComplete(savePath);
    } else {
        emit connectionError("文件保存失败: " + savePath);
    }
}

// ═══════════════════════════════════════════════════════════════
// Frame capture callback
// ═══════════════════════════════════════════════════════════════

void RemoteSession::onFrameCaptured(const QByteArray &jpegData, int width, int height)
{
    if (m_state == State::Host_Connected && m_hostSocket) {
        // 背压保护：2MB 阈值（30fps 高画质约 3MB/s，缓冲约 0.7s）
        if (m_hostSocket->bytesToWrite() < 2 * 1024 * 1024) {
            sendFrame(m_hostSocket, jpegData, width, height);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// Auth helpers
// ═══════════════════════════════════════════════════════════════

QString RemoteSession::generateDeviceId() const
{
    // 使用机器唯一ID并将其哈希为短可读字符串
    QByteArray machineId = QSysInfo::machineUniqueId();
    if (machineId.isEmpty()) {
        // 回退：随机十六进制字符串
        quint32 r = QRandomGenerator::global()->generate();// 生成一个随机 32 位整数，如 3847562918
        return QString::number(r % 1000000).rightJustified(6, '0');
    }
    QByteArray hash = QCryptographicHash::hash(machineId, QCryptographicHash::Md5);
    return hash.toHex().left(6).toUpper();
}

QString RemoteSession::generatePassword() const
{
    const QString chars("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
    QString password;
    password.reserve(6);
    for (int i = 0; i < 6; ++i) {
        password.append(chars.at(QRandomGenerator::global()->bounded(chars.size())));
    }
    return password;
}

// ═══════════════════════════════════════════════════════════════
// State accessors
// ═══════════════════════════════════════════════════════════════

QString RemoteSession::deviceId() const       { return m_deviceId; }
QString RemoteSession::currentPassword() const { return m_password; }
bool RemoteSession::isHosting() const          { return m_state == State::Host_Listening || m_state == State::Host_Connected; }
bool RemoteSession::isConnected() const        { return m_state == State::Client_Connected || m_state == State::Host_Connected; }
