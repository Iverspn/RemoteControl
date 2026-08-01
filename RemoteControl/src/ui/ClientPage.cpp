#include "ClientPage.h"
#include "RemoteView.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

ClientPage::ClientPage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setStyleSheet("background: transparent;");
    setupUI();
}

void ClientPage::setupUI()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 12, 20, 12);
    rootLayout->setSpacing(10);

    // ── 顶部 ──
    auto *headerLayout = new QHBoxLayout();
    m_backBtn = new QPushButton("← 返回首页", this);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent; color: rgba(255,255,255,0.8);
            border: 1px solid rgba(255,255,255,0.25); border-radius: 8px;
            font-size: 12px; padding: 5px 14px; font-weight: 600;
        }
        QPushButton:hover { background: rgba(255,255,255,0.08); border-color: rgba(255,255,255,0.45); color: #fff; }
    )");
    headerLayout->addWidget(m_backBtn);

    m_statusLabel = new QLabel("● 未连接", this);
    m_statusLabel->setStyleSheet("font-size: 14px; color: rgba(255,255,255,0.65); background: transparent; font-weight: 600;");
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addStretch();
    rootLayout->addLayout(headerLayout);

    // ── 连接栏（透明） ──
    auto *connBar = new QFrame(this);
    connBar->setStyleSheet("QFrame { background: transparent; border: none; }");

    auto *connLayout = new QHBoxLayout(connBar);
    connLayout->setContentsMargins(0, 4, 0, 4);
    connLayout->setSpacing(12);

    auto inputStyle = QStringLiteral(
        "border: 1px solid rgba(255,255,255,0.25); border-radius: 8px; padding: 7px 12px; "
        "font-size: 13px; background: rgba(255,255,255,0.08); color: #FFFFFF;"
        "min-height: 20px;");
    auto inputFocusStyle = QStringLiteral(
        "QLineEdit:focus, QSpinBox:focus { border-color: #FF5722; background: rgba(255,255,255,0.14); }");

    auto *ipLabel = new QLabel("IP", connBar);
    ipLabel->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.65); border: none; font-weight: 700;");
    m_hostEdit = new QLineEdit(connBar);
    m_hostEdit->setPlaceholderText("192.168.1.100");
    m_hostEdit->setMinimumWidth(130);
    m_hostEdit->setStyleSheet(inputStyle + inputFocusStyle);

    auto *portLabel = new QLabel("端口", connBar);
    portLabel->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.65); border: none; font-weight: 700;");
    m_portSpin = new QSpinBox(connBar);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(19527);
    m_portSpin->setMinimumWidth(90);
    m_portSpin->setStyleSheet(inputStyle + inputFocusStyle);

    auto *pwLabel = new QLabel("密码", connBar);
    pwLabel->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.65); border: none; font-weight: 700;");
    m_passwordEdit = new QLineEdit(connBar);
    m_passwordEdit->setPlaceholderText("输入密码");
    m_passwordEdit->setMaxLength(10);
    m_passwordEdit->setMinimumWidth(100);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setStyleSheet(inputStyle + inputFocusStyle);

    m_connectBtn = new QPushButton("连 接", connBar);
    m_connectBtn->setCursor(Qt::PointingHandCursor);
    m_connectBtn->setMinimumSize(100, 36);
    m_connectBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #FF5722,stop:1 #E64A19);
            color: #FFFFFF; border: none;
            border-radius: 8px; font-size: 14px; font-weight: 700;
        }
        QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #F4511E,stop:1 #D84315); }
    )");

    connLayout->addWidget(ipLabel);
    connLayout->addWidget(m_hostEdit, 1);
    connLayout->addWidget(portLabel);
    connLayout->addWidget(m_portSpin);
    connLayout->addWidget(pwLabel);
    connLayout->addWidget(m_passwordEdit, 1);
    connLayout->addWidget(m_connectBtn);
    rootLayout->addWidget(connBar);

    // ── RemoteView ──
    m_remoteView = new RemoteView(this);
    m_remoteView->setStyleSheet(
        "RemoteView { background: rgba(0,0,0,0.5); "
        "border: 1px solid rgba(255,255,255,0.2); border-radius: 10px; }");
    rootLayout->addWidget(m_remoteView, 1);

    // ── 底部工具栏（透明） ──
    auto *bottomBar = new QFrame(this);
    bottomBar->setStyleSheet("QFrame { background: transparent; border: none; }");

    auto *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 4, 0, 4);
    bottomLayout->setSpacing(14);

    auto *qualLabel = new QLabel("画质", bottomBar);
    qualLabel->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.65); border: none; font-weight: 700;");
    m_qualitySlider = new QSlider(Qt::Horizontal, bottomBar);
    m_qualitySlider->setRange(10, 100);
    m_qualitySlider->setValue(50);
    m_qualitySlider->setMaximumWidth(100);
    m_qualityLabel = new QLabel("50%", bottomBar);
    m_qualityLabel->setStyleSheet("font-size: 13px; color: #FF8A65; border: none; min-width: 36px; font-weight: 700; background: transparent;");

    auto *fpsLabel = new QLabel("帧率", bottomBar);
    fpsLabel->setStyleSheet("font-size: 12px; color: rgba(255,255,255,0.65); border: none; font-weight: 700;");
    m_fpsSpin = new QSpinBox(bottomBar);
    m_fpsSpin->setRange(1, 30);
    m_fpsSpin->setValue(10);
    m_fpsSpin->setSuffix(" fps");
    m_fpsSpin->setMaximumWidth(80);

    m_sendFileBtn = new QPushButton("📤 发送文件", bottomBar);
    m_sendFileBtn->setCursor(Qt::PointingHandCursor);
    m_sendFileBtn->setStyleSheet(R"(
        QPushButton { background: transparent; color: rgba(255,255,255,0.7);
                      border: 1px solid rgba(255,255,255,0.2); border-radius: 8px;
                      padding: 5px 14px; font-size: 12px; font-weight: 600; }
        QPushButton:hover { background: rgba(255,87,34,0.18); border-color: #FF5722; color: #FF8A65; }
    )");

    m_receiveFileBtn = new QPushButton("📥 接收文件", bottomBar);
    m_receiveFileBtn->setCursor(Qt::PointingHandCursor);
    m_receiveFileBtn->setStyleSheet(R"(
        QPushButton { background: transparent; color: rgba(255,255,255,0.5);
                      border: 1px solid rgba(255,255,255,0.15); border-radius: 8px;
                      padding: 5px 14px; font-size: 12px; font-weight: 600; }
        QPushButton:hover { background: rgba(255,255,255,0.08); border-color: rgba(255,255,255,0.35); color: rgba(255,255,255,0.85); }
    )");

    bottomLayout->addWidget(qualLabel);
    bottomLayout->addWidget(m_qualitySlider);
    bottomLayout->addWidget(m_qualityLabel);
    bottomLayout->addSpacing(8);
    bottomLayout->addWidget(fpsLabel);
    bottomLayout->addWidget(m_fpsSpin);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_sendFileBtn);
    bottomLayout->addWidget(m_receiveFileBtn);
    rootLayout->addWidget(bottomBar);

    // ── 信号 ──
    connect(m_backBtn, &QPushButton::clicked, this, &ClientPage::backRequested);

    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int val) {
        m_qualityLabel->setText(QString::number(val) + "%");
        if (m_connected) emit qualityChanged(val);
    });

    connect(m_fpsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_connected) emit fpsChanged(val);
    });

    connect(m_connectBtn, &QPushButton::clicked, this, [this]() {
        if (!m_connected) {
            QString host = m_hostEdit->text().trimmed();
            if (host.isEmpty()) host = "127.0.0.1";
            quint16 port = static_cast<quint16>(m_portSpin->value());
            QString password = m_passwordEdit->text().trimmed();
            if (password.isEmpty()) {
                QMessageBox::warning(this, "提示", "请输入连接密码");
                return;
            }
            emit connectRequested(host, port, "", password);
        } else {
            emit disconnectRequested();
        }
    });

    connect(m_sendFileBtn, &QPushButton::clicked, this, [this]() {
        if (!m_connected) { QMessageBox::warning(this, "提示", "请先连接到远程设备"); return; }
        QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件");
        if (!filePath.isEmpty()) emit sendFileRequested(filePath);
    });

    connect(m_receiveFileBtn, &QPushButton::clicked, this, [this]() {
        if (!m_connected) { QMessageBox::warning(this, "提示", "请先连接到远程设备"); return; }
        QString localSaveDir = QFileDialog::getExistingDirectory(this, "选择保存目录");
        if (!localSaveDir.isEmpty()) emit receiveFileRequested("/", localSaveDir);
    });
}

RemoteView *ClientPage::remoteView() const { return m_remoteView; }

void ClientPage::onConnected()
{
    m_connected = true;
    m_connectBtn->setText("断 开");
    m_connectBtn->setStyleSheet(R"(
        QPushButton { background: #EF4444; color: #FFF; border: none;
                      border-radius: 8px; font-size: 14px; font-weight: 700; }
        QPushButton:hover { background: #DC2626; }
    )");
    m_statusLabel->setText("● 已连接 · 远程控制中");
    m_statusLabel->setStyleSheet("font-size: 14px; color: #4ADE80; background: transparent; font-weight: 700;");
    m_hostEdit->setEnabled(false);
    m_portSpin->setEnabled(false);
    m_passwordEdit->setEnabled(false);
}

void ClientPage::onDisconnected()
{
    m_connected = false;
    m_connectBtn->setText("连 接");
    m_connectBtn->setStyleSheet(R"(
        QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0#FF5722,stop:1#E64A19);
                      color: #FFF; border: none; border-radius: 8px; font-size: 14px; font-weight: 700; }
        QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0#F4511E,stop:1#D84315); }
    )");
    m_statusLabel->setText("● 未连接");
    m_statusLabel->setStyleSheet("font-size: 14px; color: rgba(255,255,255,0.65); background: transparent; font-weight: 600;");
    m_hostEdit->setEnabled(true);
    m_portSpin->setEnabled(true);
    m_passwordEdit->setEnabled(true);
    m_remoteView->clearFrame();
}

void ClientPage::onFrameReceived(const QByteArray &jpegData, int width, int height)
{
    Q_UNUSED(width) Q_UNUSED(height)
    m_remoteView->updateFrame(jpegData);
}

void ClientPage::onAuthFailed(const QString &reason)
{
    QMessageBox::warning(this, "认证失败", "密码验证失败：" + reason);
}

void ClientPage::onConnectionError(const QString &msg)
{
    QMessageBox::warning(this, "连接错误", msg);
}
