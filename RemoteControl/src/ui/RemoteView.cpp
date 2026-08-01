#include "RemoteView.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QtConcurrent>

#include <windows.h>

RemoteView::RemoteView(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);

    m_decodeWatcher = new QFutureWatcher<QPixmap>(this);
    connect(m_decodeWatcher, &QFutureWatcher<QPixmap>::finished, this, [this]() {
        QPixmap decoded = m_decodeWatcher->result();
        if (decoded.isNull()) return;
        {
            QMutexLocker lock(&m_frameMutex);
            m_frame = decoded;
        }
        recalcScale();
        update();
    });
}

void RemoteView::updateFrame(const QByteArray &jpegData)
{
    // 异步 JPEG 解码，不阻塞 UI 线程
    auto future = QtConcurrent::run([jpegData]() -> QPixmap {
        QPixmap pix;
        pix.loadFromData(jpegData, "JPEG");
        return pix;
    });
    m_decodeWatcher->setFuture(future);
}

void RemoteView::clearFrame()
{
    QMutexLocker lock(&m_frameMutex);
    m_frame = QPixmap();
    update();
}

// ── Paint ──

void RemoteView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) {
        lock.unlock();
        painter.fillRect(rect(), QColor("#1A1A2E"));
        painter.setPen(QColor("#8F959E"));
        QFont font = painter.font();
        font.setPointSize(14);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, "等待远程画面...");
        return;
    }

    painter.drawPixmap(m_offsetX, m_offsetY, m_drawW, m_drawH, m_frame);
    lock.unlock();

    // 本地光标：即时跟随鼠标，不受远程帧延迟影响
    if (m_showLocalCursor) {
        painter.setPen(QPen(Qt::black, 1));
        painter.setBrush(QColor(255, 255, 255, 200));
        painter.drawEllipse(m_localCursorPos, 8, 8);
    }
}

void RemoteView::resizeEvent(QResizeEvent *)
{
    m_lastWidgetSize = QSize(); // 强制重算
    recalcScale();
}

void RemoteView::recalcScale()
{
    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) return;

    // 缓存：尺寸没变就跳过
    if (size() == m_lastWidgetSize && m_frame.size() == m_lastFrameSize) return;
    m_lastWidgetSize = size();
    m_lastFrameSize = m_frame.size();

    QSize scaled = m_frame.size().scaled(size(), Qt::KeepAspectRatio);
    m_drawW = scaled.width();
    m_drawH = scaled.height();
    m_offsetX = (width() - m_drawW) / 2;
    m_offsetY = (height() - m_drawH) / 2;

    // 预计算缩放比例，避免每次鼠标事件重复除法
    int fw = m_frame.width(), fh = m_frame.height();
    m_scaleX = (m_drawW > 1) ? static_cast<qreal>(fw - 1) / (m_drawW - 1) : 0;
    m_scaleY = (m_drawH > 1) ? static_cast<qreal>(fh - 1) / (m_drawH - 1) : 0;
}

// ── 判断点是否在画面区域内 ──
static bool inDrawArea(const QPoint &localPos,
                       int offsetX, int offsetY, int drawW, int drawH)
{
    return localPos.x() >= offsetX && localPos.x() <= offsetX + drawW
        && localPos.y() >= offsetY && localPos.y() <= offsetY + drawH;
}

// ── 快速坐标映射（预计算的 scaleX/Y） ──
QPoint RemoteView::mapToRemoteFast(const QPoint &localPos) const
{
    int rx = qRound((localPos.x() - m_offsetX) * m_scaleX);
    int ry = qRound((localPos.y() - m_offsetY) * m_scaleY);
    return QPoint(qBound(0, rx, m_frame.width() - 1),
                  qBound(0, ry, m_frame.height() - 1));
}

void RemoteView::mousePressEvent(QMouseEvent *event)
{
    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) return;
    if (!inDrawArea(event->pos(), m_offsetX, m_offsetY, m_drawW, m_drawH)) return;
    QPoint remote = mapToRemoteFast(event->pos());
    lock.unlock();
    emit mousePressed(remote.x(), remote.y(), event->button());
    setFocus();
}

void RemoteView::mouseReleaseEvent(QMouseEvent *event)
{
    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) return;
    if (!inDrawArea(event->pos(), m_offsetX, m_offsetY, m_drawW, m_drawH)) return;
    QPoint remote = mapToRemoteFast(event->pos());
    lock.unlock();
    emit mouseReleased(remote.x(), remote.y(), event->button());
}

void RemoteView::mouseMoveEvent(QMouseEvent *event)
{
    m_localCursorPos = event->pos();
    m_showLocalCursor = true;

    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) return;
    if (!inDrawArea(event->pos(), m_offsetX, m_offsetY, m_drawW, m_drawH)) return;
    QPoint remote = mapToRemoteFast(event->pos());
    lock.unlock();
    emit mouseMoved(remote.x(), remote.y());
    update(); // 只重绘光标
}

void RemoteView::wheelEvent(QWheelEvent *event)
{
    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) return;
    lock.unlock();
    emit mouseWheeled(event->angleDelta().y());
}

// ── Keyboard events → Win32 VK codes ──

int RemoteView::mapQtKeyToVk(int qtKey) const
{
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) return qtKey;
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) return qtKey;
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
        return VK_F1 + (qtKey - Qt::Key_F1);

    switch (qtKey) {
    case Qt::Key_Return:     case Qt::Key_Enter:    return VK_RETURN;
    case Qt::Key_Backspace:  return VK_BACK;
    case Qt::Key_Tab:        return VK_TAB;
    case Qt::Key_Escape:     return VK_ESCAPE;
    case Qt::Key_Space:      return VK_SPACE;
    case Qt::Key_Left:       return VK_LEFT;
    case Qt::Key_Right:      return VK_RIGHT;
    case Qt::Key_Up:         return VK_UP;
    case Qt::Key_Down:       return VK_DOWN;
    case Qt::Key_Shift:      return VK_SHIFT;
    case Qt::Key_Control:    return VK_CONTROL;
    case Qt::Key_Alt:        return VK_MENU;
    case Qt::Key_Meta:       return VK_LWIN;
    case Qt::Key_Delete:     return VK_DELETE;
    case Qt::Key_Insert:     return VK_INSERT;
    case Qt::Key_Home:       return VK_HOME;
    case Qt::Key_End:        return VK_END;
    case Qt::Key_PageUp:     return VK_PRIOR;
    case Qt::Key_PageDown:   return VK_NEXT;
    case Qt::Key_CapsLock:   return VK_CAPITAL;
    case Qt::Key_NumLock:    return VK_NUMLOCK;
    case Qt::Key_ScrollLock: return VK_SCROLL;
    case Qt::Key_Pause:      return VK_PAUSE;
    // Numpad
    case Qt::Key_0 + Qt::KeypadModifier: return VK_NUMPAD0;
    case Qt::Key_1 + Qt::KeypadModifier: return VK_NUMPAD1;
    case Qt::Key_2 + Qt::KeypadModifier: return VK_NUMPAD2;
    case Qt::Key_3 + Qt::KeypadModifier: return VK_NUMPAD3;
    case Qt::Key_4 + Qt::KeypadModifier: return VK_NUMPAD4;
    case Qt::Key_5 + Qt::KeypadModifier: return VK_NUMPAD5;
    case Qt::Key_6 + Qt::KeypadModifier: return VK_NUMPAD6;
    case Qt::Key_7 + Qt::KeypadModifier: return VK_NUMPAD7;
    case Qt::Key_8 + Qt::KeypadModifier: return VK_NUMPAD8;
    case Qt::Key_9 + Qt::KeypadModifier: return VK_NUMPAD9;
    case Qt::Key_Comma:      return VK_OEM_COMMA;
    case Qt::Key_Period:     return VK_OEM_PERIOD;
    case Qt::Key_Slash:      return VK_OEM_2;
    case Qt::Key_Semicolon:  return VK_OEM_1;
    case Qt::Key_BracketLeft:  return VK_OEM_4;
    case Qt::Key_BracketRight: return VK_OEM_6;
    case Qt::Key_Backslash:  return VK_OEM_5;
    case Qt::Key_QuoteLeft:  return VK_OEM_3;
    case Qt::Key_Apostrophe: return VK_OEM_7;
    case Qt::Key_Minus:      return VK_OEM_MINUS;
    case Qt::Key_Equal:      return VK_OEM_PLUS;
    default: return 0;
    }
}

void RemoteView::keyPressEvent(QKeyEvent *event)
{
    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) { QWidget::keyPressEvent(event); return; }
    lock.unlock();
    int vk = mapQtKeyToVk(event->key());
    if (vk != 0) emit keyPressed(vk);
}

void RemoteView::keyReleaseEvent(QKeyEvent *event)
{
    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isNull()) { QWidget::keyReleaseEvent(event); return; }
    lock.unlock();
    if (event->isAutoRepeat()) return;
    int vk = mapQtKeyToVk(event->key());
    if (vk != 0) emit keyReleased(vk);
}
