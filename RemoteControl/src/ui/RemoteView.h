#ifndef REMOTEVIEW_H
#define REMOTEVIEW_H

#include <QWidget>
#include <QPixmap>
#include <QFutureWatcher>
#include <QMutex>
#include <QElapsedTimer>

class RemoteView : public QWidget
{
    Q_OBJECT
public:
    explicit RemoteView(QWidget *parent = nullptr);

    void updateFrame(const QByteArray &jpegData);
    void clearFrame();

signals:
    void mouseMoved(int remoteX, int remoteY);
    void mousePressed(int remoteX, int remoteY, Qt::MouseButton button);
    void mouseReleased(int remoteX, int remoteY, Qt::MouseButton button);
    void mouseWheeled(int delta);
    void keyPressed(int vkCode);
    void keyReleased(int vkCode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void recalcScale();
    QPoint mapToRemoteFast(const QPoint &localPos) const;
    int  mapQtKeyToVk(int qtKey) const;

    QPixmap m_frame;
    QMutex  m_frameMutex;
    int     m_offsetX   = 0;
    int     m_offsetY   = 0;
    int     m_drawW     = 0;
    int     m_drawH     = 0;

    // 缩放缓存
    QSize m_lastWidgetSize;
    QSize m_lastFrameSize;
    qreal m_scaleX  = 1.0;
    qreal m_scaleY  = 1.0;

    // 本地光标（即时反馈）
    QPoint m_localCursorPos;
    bool   m_showLocalCursor = false;

    // 异步 JPEG 解码
    QFutureWatcher<QPixmap> *m_decodeWatcher = nullptr;
};

#endif // REMOTEVIEW_H
