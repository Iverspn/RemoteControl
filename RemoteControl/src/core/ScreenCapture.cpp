#include "ScreenCapture.h"

#include <QGuiApplication>
#include <QPixmap>
#include <QImage>
#include <QBuffer>
#include <QElapsedTimer>
#include <QtMath>
#include <QtConcurrent>
#include <QFutureWatcher>

ScreenCapture::ScreenCapture(QObject *parent)
    : QObject(parent)
    , m_screen(QGuiApplication::primaryScreen())
{
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &ScreenCapture::onTimer);
    m_encodeTimer = new QElapsedTimer();
}

void ScreenCapture::start()
{
    if (m_active) return;
    m_screen = QGuiApplication::primaryScreen();
    if (!m_screen) {
        emit captureError("无法获取主屏幕");
        return;
    }
    m_active = true;
    m_encoding = false;
    updateTimerInterval();
}

void ScreenCapture::stop()
{
    m_active = false;
    m_timer->stop();
}

bool ScreenCapture::isActive() const { return m_active; }

void ScreenCapture::setQuality(int quality)
{
    m_quality = qBound(10, quality, 100);
}

void ScreenCapture::setFps(int fps)
{
    m_fps = qBound(1, fps, 30);
    if (m_active) updateTimerInterval();
}

int ScreenCapture::quality() const { return m_quality; }
int ScreenCapture::fps() const     { return m_fps; }
qreal ScreenCapture::captureScale() const { return m_currentScale; }

void ScreenCapture::updateTimerInterval()
{
    m_timer->start(1000 / m_fps);
}

void ScreenCapture::onTimer()
{
    if (!m_active) return;
    if (m_encoding) return; // 上一帧还没编完，跳帧
    m_encoding = true;

    m_encodeTimer->restart();

    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) { m_encoding = false; return; }

    QPixmap pixmap = screen->grabWindow(0);
    if (pixmap.isNull()) { m_encoding = false; return; }

    // 自适应缩放：帧率越高，分辨率越低，保持编码时间可控
    //  1-10fps → 全分辨率（100ms+ 充裕）
    // 11-20fps → 3/4 分辨率（50-90ms）
    // 21-25fps → 1/2 分辨率（40-48ms）
    // 26-30fps → 2/5 分辨率（33-38ms）
    qreal scale = 1.0;
    if      (m_fps >= 26) scale = 0.4;  // 2/5
    else if (m_fps >= 21) scale = 0.5;  // 1/2
    else if (m_fps >= 11) scale = 0.75; // 3/4
    m_currentScale = scale;

    // QImage 优化：转为 RGB888（比 ARGB32 少 25% 数据量，编码更快）
    QImage image = pixmap.toImage().convertToFormat(QImage::Format_RGB888);
    if (scale < 1.0) {
        image = image.scaled(image.size() * scale, Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation);
    }

    int w = image.width(), h = image.height();

    // 异步 JPEG 编码，不阻塞主线程
    auto future = QtConcurrent::run([image = std::move(image), q = m_quality]() -> QByteArray {
        QByteArray jpegData;
        QBuffer buffer(&jpegData);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPEG", q);
        return jpegData;
    });

    auto *watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [this, watcher, w, h]() {
        QByteArray jpegData = watcher->result();
        m_encoding = false;

        int elapsed = static_cast<int>(m_encodeTimer->elapsed());
        if (elapsed > 1000 / m_fps) {
            m_consecutiveSlow++;
        } else {
            m_consecutiveSlow = 0;
        }
        if (m_consecutiveSlow > 5) {
            qWarning() << "ScreenCapture: persistent slow encode, consider reducing fps";
        }

        if (!jpegData.isEmpty())
            emit frameCaptured(jpegData, w, h);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}
