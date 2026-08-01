#include "HostPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTimer>
#include <QtMath>

HostPage::HostPage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setStyleSheet("background: transparent;");
    setupUI();
}

void HostPage::setupUI()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(60, 28, 60, 28);
    rootLayout->setSpacing(18);

    // ── 顶部标题栏 ──
    auto *headerLayout = new QHBoxLayout();
    m_backBtn = new QPushButton("← 返回首页", this);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent; color: rgba(255,255,255,0.85);
            border: 1px solid rgba(255,255,255,0.3);
            border-radius: 8px; font-size: 13px; padding: 6px 16px;
        }
        QPushButton:hover {
            background: rgba(255,255,255,0.1);
            border-color: rgba(255,255,255,0.5); color: #FFFFFF;
        }
    )");
    headerLayout->addWidget(m_backBtn);
    headerLayout->addStretch();

    auto *title = new QLabel("被控端 · 等待远程连接", this);
    title->setStyleSheet("font-size: 22px; font-weight: 800; color: #FFFFFF; background: transparent;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addSpacing(100);
    rootLayout->addLayout(headerLayout);

    rootLayout->addSpacing(10);

    // ═══════════════════════════════════════════════════════
    // 设备信息区（无卡片背景，透明）
    // ═══════════════════════════════════════════════════════
    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(16);

    // 设备 ID
    auto *idRow = new QHBoxLayout();
    auto *idTitle = new QLabel("设备 ID", this);
    idTitle->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.6); "
                           "background: transparent; font-weight: 600; letter-spacing: 2px;");
    m_deviceIdLabel = new QLabel(this);
    m_deviceIdLabel->setStyleSheet(
        "font-size: 22px; font-weight: 800; color: #FF6D3F; "
        "background: transparent; letter-spacing: 4px; "
        "font-family: 'Consolas', 'Microsoft YaHei', monospace;");
    idRow->addWidget(idTitle);
    idRow->addWidget(m_deviceIdLabel);
    idRow->addStretch();
    infoLayout->addLayout(idRow);

    // 密码
    auto *pwRow = new QHBoxLayout();
    auto *pwTitle = new QLabel("连接密码", this);
    pwTitle->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.6); "
                           "background: transparent; font-weight: 600; letter-spacing: 2px;");
    m_passwordLabel = new QLabel(this);
    m_passwordLabel->setStyleSheet(
        "font-size: 28px; font-weight: 800; color: #FFFFFF; "
        "letter-spacing: 6px; background: transparent;");

    m_copyBtn = new QPushButton("📋 复制", this);
    m_copyBtn->setCursor(Qt::PointingHandCursor);
    m_copyBtn->setStyleSheet(R"(
        QPushButton { background: transparent; color: rgba(255,255,255,0.7);
                      border: 1px solid rgba(255,255,255,0.25); border-radius: 8px;
                      padding: 6px 16px; font-size: 12px; font-weight: 600; }
        QPushButton:hover { background: rgba(255,87,34,0.2); border-color: #FF5722; color: #FF8A65; }
    )");

    m_regenerateBtn = new QPushButton("🔄 刷新", this);
    m_regenerateBtn->setCursor(Qt::PointingHandCursor);
    m_regenerateBtn->setStyleSheet(R"(
        QPushButton { background: transparent; color: rgba(255,255,255,0.5);
                      border: 1px solid rgba(255,255,255,0.15); border-radius: 8px;
                      padding: 6px 16px; font-size: 12px; font-weight: 600; }
        QPushButton:hover { background: rgba(255,255,255,0.08); border-color: rgba(255,255,255,0.35); color: rgba(255,255,255,0.8); }
    )");

    pwRow->addWidget(pwTitle);
    pwRow->addWidget(m_passwordLabel);
    pwRow->addWidget(m_copyBtn);
    pwRow->addWidget(m_regenerateBtn);
    pwRow->addStretch();
    infoLayout->addLayout(pwRow);

    rootLayout->addLayout(infoLayout);

    rootLayout->addSpacing(14);

    // ═══════════════════════════════════════════════════════
    // 参数设置区（透明）
    // ═══════════════════════════════════════════════════════
    auto *settingsLayout = new QVBoxLayout();
    settingsLayout->setSpacing(16);

    auto *settingsTitle = new QLabel("参 数 设 置", this);
    settingsTitle->setStyleSheet("font-size: 13px; font-weight: 700; color: rgba(255,255,255,0.5); "
                                 "background: transparent; letter-spacing: 4px;");
    settingsLayout->addWidget(settingsTitle);

    // Port
    auto *portRow = new QHBoxLayout();
    auto *portLabel = new QLabel("监听端口", this);
    portLabel->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.7); background: transparent;");
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(19527);
    m_portSpin->setMinimumWidth(100);
    portRow->addWidget(portLabel);
    portRow->addWidget(m_portSpin);
    portRow->addStretch();
    settingsLayout->addLayout(portRow);

    // Quality
    auto *qualRow = new QHBoxLayout();
    auto *qualLabel = new QLabel("画面质量", this);
    qualLabel->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.7); background: transparent;");
    m_qualitySlider = new QSlider(Qt::Horizontal, this);
    m_qualitySlider->setRange(10, 100);
    m_qualitySlider->setValue(50);
    m_qualityLabel = new QLabel("50%", this);
    m_qualityLabel->setStyleSheet("font-size: 14px; color: #FF8A65; border: none; "
                                  "min-width: 40px; font-weight: 700; background: transparent;");
    qualRow->addWidget(qualLabel);
    qualRow->addWidget(m_qualitySlider);
    qualRow->addWidget(m_qualityLabel);
    settingsLayout->addLayout(qualRow);

    // FPS
    auto *fpsRow = new QHBoxLayout();
    auto *fpsLabel = new QLabel("帧　　率", this);
    fpsLabel->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.7); background: transparent;");
    m_fpsSpin = new QSpinBox(this);
    m_fpsSpin->setRange(1, 30);
    m_fpsSpin->setValue(10);
    m_fpsSpin->setSuffix(" fps");
    m_fpsSpin->setMinimumWidth(100);
    fpsRow->addWidget(fpsLabel);
    fpsRow->addWidget(m_fpsSpin);
    fpsRow->addStretch();
    settingsLayout->addLayout(fpsRow);

    rootLayout->addLayout(settingsLayout);

    rootLayout->addSpacing(8);

    // ── 控制按钮 ──
    m_startStopBtn = new QPushButton("开 始 监 听", this);
    m_startStopBtn->setMinimumHeight(52);
    m_startStopBtn->setCursor(Qt::PointingHandCursor);
    m_startStopBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #FF5722, stop:1 #E64A19);
            color: #FFFFFF; border: none;
            border-radius: 14px; font-size: 17px; font-weight: 800;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #F4511E, stop:1 #D84315);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #D84315, stop:1 #BF360C);
        }
        QPushButton:disabled { background: #666; }
    )");
    rootLayout->addWidget(m_startStopBtn);

    // ── 状态 ──
    m_statusLabel = new QLabel("● 未启动", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 15px; color: rgba(255,255,255,0.6); background: transparent;");
    rootLayout->addWidget(m_statusLabel);

    m_clientLabel = new QLabel(this);
    m_clientLabel->setAlignment(Qt::AlignCenter);
    m_clientLabel->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.7); background: transparent;");
    m_clientLabel->hide();
    rootLayout->addWidget(m_clientLabel);

    rootLayout->addStretch();

    // ── 信号 ──
    connect(m_backBtn, &QPushButton::clicked, this, &HostPage::backRequested);

    connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int val) {
        m_qualityLabel->setText(QString::number(val) + "%");
    });

    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_currentPassword);
        m_copyBtn->setText("✓ 已复制");
        m_copyBtn->setStyleSheet(
            "QPushButton { background: rgba(34,197,94,0.2); color: #4ADE80; "
            "border: 1px solid rgba(34,197,94,0.4); border-radius: 8px; "
            "padding: 6px 16px; font-size: 12px; font-weight: 600; }");
        QTimer::singleShot(2000, this, [this](){
            m_copyBtn->setText("📋 复制");
            m_copyBtn->setStyleSheet(R"(
                QPushButton { background: transparent; color: rgba(255,255,255,0.7);
                              border: 1px solid rgba(255,255,255,0.25); border-radius: 8px;
                              padding: 6px 16px; font-size: 12px; font-weight: 600; }
                QPushButton:hover { background: rgba(255,87,34,0.2); border-color: #FF5722; color: #FF8A65; }
            )");
        });
    });

    connect(m_regenerateBtn, &QPushButton::clicked, this, [this]() {
        emit regeneratePassword(); // MainWindow 会回调 setPassword()
    });

    connect(m_startStopBtn, &QPushButton::clicked, this, [this]() {
        if (!m_isHosting) {
            emit startHostRequested(m_portSpin->value(), m_currentPassword,
                                     m_qualitySlider->value(), m_fpsSpin->value());
        } else {
            emit stopHostRequested();
        }
    });
}

void HostPage::setDeviceId(const QString &id) { m_deviceIdLabel->setText(id); }

void HostPage::setPassword(const QString &pwd)
{
    m_currentPassword = pwd;
    m_passwordLabel->setText(pwd);
}

void HostPage::onHostStarted()
{
    m_isHosting = true;
    m_startStopBtn->setText("停 止 监 听");
    m_startStopBtn->setStyleSheet(R"(
        QPushButton { background: #EF4444; color: #FFFFFF; border: none;
                      border-radius: 14px; font-size: 17px; font-weight: 800; }
        QPushButton:hover { background: #DC2626; }
    )");
    m_statusLabel->setText("● 正在监听 · 等待连接中...");
    m_statusLabel->setStyleSheet("font-size: 15px; color: #FF8A65; background: transparent; font-weight: 700;");
    m_clientLabel->hide();
    m_portSpin->setEnabled(false);
    m_regenerateBtn->setEnabled(false);
}

void HostPage::onHostStopped()
{
    m_isHosting = false;
    m_startStopBtn->setText("开 始 监 听");
    m_startStopBtn->setStyleSheet(R"(
        QPushButton { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF5722,stop:1 #E64A19);
                      color: #FFFFFF; border: none; border-radius: 14px;
                      font-size: 17px; font-weight: 800; }
        QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #F4511E,stop:1 #D84315); }
    )");
    m_statusLabel->setText("● 未启动");
    m_statusLabel->setStyleSheet("font-size: 15px; color: rgba(255,255,255,0.6); background: transparent;");
    m_clientLabel->hide();
    m_portSpin->setEnabled(true);
    m_regenerateBtn->setEnabled(true);
}

void HostPage::onClientConnected(const QString &clientIp)
{
    m_statusLabel->setText("● 已连接 · 正在远程控制中");
    m_statusLabel->setStyleSheet("font-size: 15px; color: #4ADE80; background: transparent; font-weight: 700;");
    m_clientLabel->setText("客户端 IP：" + clientIp);
    m_clientLabel->setStyleSheet("font-size: 13px; color: rgba(255,255,255,0.8); background: transparent;");
    m_clientLabel->show();
}

void HostPage::onClientDisconnected()
{
    m_statusLabel->setText("● 正在监听 · 等待连接中...");
    m_statusLabel->setStyleSheet("font-size: 15px; color: #FF8A65; background: transparent; font-weight: 700;");
    m_clientLabel->hide();
}

void HostPage::onConnectionError(const QString &msg)
{
    m_statusLabel->setText("✕ " + msg);
    m_statusLabel->setStyleSheet("font-size: 15px; color: #FCA5A5; background: transparent; font-weight: 700;");
}

