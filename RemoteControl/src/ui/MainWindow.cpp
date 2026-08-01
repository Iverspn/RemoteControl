#include "MainWindow.h"
#include "HomePage.h"
#include "HostPage.h"
#include "ClientPage.h"
#include "RemoteView.h"
#include "core/RemoteSession.h"

#include <QVBoxLayout>
#include <QStackedWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    // ── 无边框窗口 ──
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // ── 加载背景图 ──
    m_background.load(":/picture/background.jpg");

    setupUI();
    setupConnections();
}

// ── 背景图绘制 ──
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!m_background.isNull()) {
        QPixmap scaled = m_background.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                              Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        painter.drawPixmap(x, y, scaled);
    } else {
        painter.fillRect(rect(), QColor("#1A1A2E"));
    }
}

// ── 无边框窗口拖拽 ──
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_dragging = false;
}

void MainWindow::setupUI()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    m_pages = new QStackedWidget(this);
    m_pages->setStyleSheet("QStackedWidget { background: transparent; }");

    m_homePage   = new HomePage(this);
    m_hostPage   = new HostPage(this);
    m_clientPage = new ClientPage(this);

    m_pages->addWidget(m_homePage);
    m_pages->addWidget(m_hostPage);
    m_pages->addWidget(m_clientPage);

    m_pages->setCurrentWidget(m_homePage);
    rootLayout->addWidget(m_pages);
}

void MainWindow::setupConnections()
{
    m_session = new RemoteSession(this);

    // ── HomePage navigation ──
    connect(m_homePage, &HomePage::hostModeRequested, this, [this]() {
        if (m_session->isHosting()) {
            QMessageBox::information(this, "提示", "当前正在作为被控端运行");
        }
        m_hostPage->setDeviceId(m_session->deviceId());
        m_hostPage->setPassword(m_session->currentPassword());
        m_pages->setCurrentWidget(m_hostPage);
    });
    connect(m_homePage, &HomePage::clientModeRequested, this, [this]() {
        if (m_session->isConnected()) {
            QMessageBox::information(this, "提示", "当前正在作为控制端运行");
        }
        m_pages->setCurrentWidget(m_clientPage);
    });

    m_homePage->setDeviceId(m_session->deviceId());

    // ── HomePage close button ──
    connect(m_homePage, &HomePage::closeAppRequested, this, &QWidget::close);
    connect(m_homePage, &HomePage::minimizeAppRequested, this, &QWidget::showMinimized);

    // ── Back buttons ──
    connect(m_hostPage, &HostPage::backRequested, this, [this]() {
        m_pages->setCurrentWidget(m_homePage);
    });
    connect(m_clientPage, &ClientPage::backRequested, this, [this]() {
        m_pages->setCurrentWidget(m_homePage);
    });

    // ── Host: UI → Session ──
    connect(m_hostPage, &HostPage::startHostRequested,  m_session, &RemoteSession::startHost);
    connect(m_hostPage, &HostPage::stopHostRequested,   m_session, &RemoteSession::stopHost);
    connect(m_hostPage, &HostPage::regeneratePassword, this, [this]() {
        m_session->regeneratePassword();
        m_hostPage->setPassword(m_session->currentPassword());
    });

    // ── Host: Session → UI ──
    connect(m_session, &RemoteSession::hostStarted,        m_hostPage, &HostPage::onHostStarted);
    connect(m_session, &RemoteSession::hostStopped,        m_hostPage, &HostPage::onHostStopped);
    connect(m_session, &RemoteSession::clientConnected,    m_hostPage, &HostPage::onClientConnected);
    connect(m_session, &RemoteSession::clientDisconnected, m_hostPage, &HostPage::onClientDisconnected);
    connect(m_session, &RemoteSession::connectionError,    m_hostPage, &HostPage::onConnectionError);

    // ── Client: UI → Session ──
    connect(m_clientPage, &ClientPage::connectRequested,    m_session, &RemoteSession::connectToHost);
    connect(m_clientPage, &ClientPage::disconnectRequested, m_session, &RemoteSession::disconnectFromHost);
    connect(m_clientPage, &ClientPage::qualityChanged,      m_session, &RemoteSession::setQuality);
    connect(m_clientPage, &ClientPage::fpsChanged,          m_session, &RemoteSession::setFps);

    // ── Client: Session → UI ──
    connect(m_session, &RemoteSession::connectedToHost,     m_clientPage, &ClientPage::onConnected);
    connect(m_session, &RemoteSession::disconnectedFromHost, m_clientPage, &ClientPage::onDisconnected);
    connect(m_session, &RemoteSession::frameReceived,       m_clientPage, &ClientPage::onFrameReceived);
    connect(m_session, &RemoteSession::authFailed,          m_clientPage, &ClientPage::onAuthFailed);

    // ── RemoteView input → Session ──
    RemoteView *rv = m_clientPage->remoteView();
    connect(rv, &RemoteView::mouseMoved, m_session, &RemoteSession::sendMouseMove);

    connect(rv, &RemoteView::mousePressed, this,
            [this](int x, int y, Qt::MouseButton btn) {
        QString btnStr;
        switch (btn) {
        case Qt::LeftButton:   btnStr = "left";   break;
        case Qt::RightButton:  btnStr = "right";  break;
        case Qt::MiddleButton: btnStr = "middle"; break;
        default: return;
        }
        m_session->sendMouseEvent(btnStr, true, x, y);
    });

    connect(rv, &RemoteView::mouseReleased, this,
            [this](int x, int y, Qt::MouseButton btn) {
        QString btnStr;
        switch (btn) {
        case Qt::LeftButton:   btnStr = "left";   break;
        case Qt::RightButton:  btnStr = "right";  break;
        case Qt::MiddleButton: btnStr = "middle"; break;
        default: return;
        }
        m_session->sendMouseEvent(btnStr, false, x, y);
    });

    connect(rv, &RemoteView::mouseWheeled, m_session, &RemoteSession::sendMouseWheel);
    connect(rv, &RemoteView::keyPressed,   m_session, &RemoteSession::sendKeyPress);
    connect(rv, &RemoteView::keyReleased,  m_session, &RemoteSession::sendKeyRelease);

    // ── File transfer ──
    connect(m_clientPage, &ClientPage::sendFileRequested, m_session, &RemoteSession::sendFile);
    connect(m_clientPage, &ClientPage::receiveFileRequested, this,
            [this](const QString &remotePath, const QString &localDir) {
        Q_UNUSED(remotePath)
        m_session->requestFile("download", localDir + "/received_file");
    });

    connect(m_session, &RemoteSession::fileReceiveComplete, this,
            [this](const QString &fileName) {
        QMessageBox::information(this, "文件传输", "文件接收完成: " + fileName);
    });

    // ── Error handling ──
    connect(m_session, &RemoteSession::connectionError, this,
            [this](const QString &msg) { QMessageBox::warning(this, "连接错误", msg); });
    connect(m_session, &RemoteSession::authFailed, this,
            [this](const QString &reason) { QMessageBox::warning(this, "认证失败", reason); });
}
