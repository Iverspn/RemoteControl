#ifndef HOSTPAGE_H
#define HOSTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>

class HostPage : public QWidget
{
    Q_OBJECT
public:
    explicit HostPage(QWidget *parent = nullptr);

    void setDeviceId(const QString &id);
    void setPassword(const QString &pwd);

signals:
    void backRequested();
    void startHostRequested(int port, const QString &password, int quality, int fps);
    void stopHostRequested();
    void regeneratePassword();

public slots:
    void onHostStarted();
    void onHostStopped();
    void onClientConnected(const QString &clientIp);
    void onClientDisconnected();
    void onConnectionError(const QString &msg);

private:
    void setupUI();

    // Display
    QLabel      *m_deviceIdLabel   = nullptr;
    QLabel      *m_passwordLabel   = nullptr;

    // Settings
    QSpinBox    *m_portSpin        = nullptr;
    QSlider     *m_qualitySlider   = nullptr;
    QLabel      *m_qualityLabel    = nullptr;
    QSpinBox    *m_fpsSpin         = nullptr;

    // Controls
    QPushButton *m_startStopBtn    = nullptr;
    QPushButton *m_regenerateBtn   = nullptr;
    QPushButton *m_copyBtn         = nullptr;
    QPushButton *m_backBtn         = nullptr;

    // Status
    QLabel      *m_statusLabel     = nullptr;
    QLabel      *m_clientLabel     = nullptr;

    // State
    bool         m_isHosting       = false;
    QString      m_currentPassword;
};

#endif // HOSTPAGE_H
