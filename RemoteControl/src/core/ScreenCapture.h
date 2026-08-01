#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QObject>
#include <QTimer>
#include <QScreen>

class ScreenCapture : public QObject
{
    Q_OBJECT
public:
    explicit ScreenCapture(QObject *parent = nullptr);

    void start();
    void stop();
    bool isActive() const;

    void setQuality(int quality);  // 10-100
    void setFps(int fps);          // 1-30
    int quality() const;
    int fps() const;
    qreal captureScale() const;

signals:
    void frameCaptured(const QByteArray &jpegData, int width, int height);
    void captureError(const QString &message);

private slots:
    void onTimer();

private:
    void updateTimerInterval();

    QTimer        *m_timer            = nullptr;
    QElapsedTimer *m_encodeTimer      = nullptr;
    QScreen       *m_screen           = nullptr;
    int            m_quality          = 50;
    int            m_fps              = 10;
    bool           m_active           = false;
    bool           m_encoding         = false;
    int            m_consecutiveSlow  = 0;
    qreal          m_currentScale     = 1.0;
};

#endif // SCREENCAPTURE_H
