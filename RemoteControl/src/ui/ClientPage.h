#ifndef CLIENTPAGE_H
#define CLIENTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>

class RemoteView;

class ClientPage : public QWidget
{
    Q_OBJECT
public:
    explicit ClientPage(QWidget *parent = nullptr);

    RemoteView *remoteView() const;

signals:
    void backRequested();
    void connectRequested(const QString &host, quint16 port,
                          const QString &deviceId, const QString &password);
    void disconnectRequested();
    void qualityChanged(int quality);
    void fpsChanged(int fps);
    void sendFileRequested(const QString &localPath);
    void receiveFileRequested(const QString &remotePath, const QString &localSavePath);

public slots:
    void onConnected();
    void onDisconnected();
    void onFrameReceived(const QByteArray &jpegData, int width, int height);
    void onAuthFailed(const QString &reason);
    void onConnectionError(const QString &msg);

private:
    void setupUI();

    // Connection form
    QLineEdit   *m_hostEdit       = nullptr;
    QSpinBox    *m_portSpin       = nullptr;
    QLineEdit   *m_deviceIdEdit   = nullptr;
    QLineEdit   *m_passwordEdit   = nullptr;
    QPushButton *m_connectBtn     = nullptr;

    // Settings
    QSlider     *m_qualitySlider  = nullptr;
    QLabel      *m_qualityLabel   = nullptr;
    QSpinBox    *m_fpsSpin        = nullptr;

    // File transfer
    QPushButton *m_sendFileBtn    = nullptr;
    QPushButton *m_receiveFileBtn = nullptr;

    // Status
    QLabel      *m_statusLabel    = nullptr;

    // Back
    QPushButton *m_backBtn        = nullptr;

    // Remote view
    RemoteView  *m_remoteView     = nullptr;

    // State
    bool         m_connected      = false;
};

#endif // CLIENTPAGE_H
