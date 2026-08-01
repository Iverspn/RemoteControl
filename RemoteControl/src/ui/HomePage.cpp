#include "HomePage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

HomePage::HomePage(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setStyleSheet("background: transparent;");

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // ═══════════════════════════════════════════════════════════
    // 顶部栏：关闭按钮
    // ═══════════════════════════════════════════════════════════
    auto *topBar = new QHBoxLayout();
    topBar->setContentsMargins(0, 6, 12, 0);
    topBar->addStretch();

    auto *minBtn = new QPushButton("─", this);
    minBtn->setFixedSize(32, 32);
    minBtn->setCursor(Qt::PointingHandCursor);
    minBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,0.15);
            color: rgba(255,255,255,0.7);
            border: none;
            border-radius: 16px;
            font-size: 18px; font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(255,255,255,0.3);
            color: #FFFFFF;
        }
    )");

    auto *closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(32, 32);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,0.15);
            color: rgba(255,255,255,0.7);
            border: none;
            border-radius: 16px;
            font-size: 16px;
        }
        QPushButton:hover {
            background: rgba(239,68,68,0.8);
            color: #FFFFFF;
        }
    )");
    topBar->addWidget(minBtn);
    topBar->addWidget(closeBtn);
    rootLayout->addLayout(topBar);

    // ── 主内容区居中 ──
    rootLayout->addStretch();

    // ═══════════════════════════════════════════════════════════
    // Logo / 标题
    // ═══════════════════════════════════════════════════════════
    auto *title = new QLabel("RemoteControl", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 38px; font-weight: 800; color: #FFFFFF; "
        "background: transparent;");
    rootLayout->addWidget(title);

    auto *subtitle = new QLabel("跨设备远程桌面控制 · 安全高效低延迟", this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(
        "font-size: 15px; color: rgba(255,255,255,0.75); "
        "background: transparent; margin-bottom: 8px;");
    rootLayout->addWidget(subtitle);

    rootLayout->addSpacing(20);

    // ═══════════════════════════════════════════════════════════
    // 设备 ID 卡片（毛玻璃）
    // ═══════════════════════════════════════════════════════════
    auto *idCard = new QFrame(this);
    idCard->setObjectName("idCard");
    idCard->setStyleSheet(R"(
        #idCard {
            background: transparent;
            border: none;
            border-radius: 0;
        }
    )");

    auto *idLayout = new QVBoxLayout(idCard);
    idLayout->setContentsMargins(52, 26, 52, 26);
    idLayout->setSpacing(8);

    auto *idHint = new QLabel("本 机 设 备  I D", idCard);
    idHint->setAlignment(Qt::AlignCenter);
    idHint->setStyleSheet(
        "font-size: 11px; color: rgba(255,255,255,0.5); border: none; "
        "letter-spacing: 6px;");
    idLayout->addWidget(idHint);

    m_deviceIdLabel = new QLabel("生成中...", idCard);
    m_deviceIdLabel->setAlignment(Qt::AlignCenter);
    m_deviceIdLabel->setStyleSheet(
        "font-size: 38px; font-weight: 700; color: #FF6D3F; "
        "letter-spacing: 8px; border: none; "
        "font-family: 'Consolas', 'Microsoft YaHei', monospace;");
    idLayout->addWidget(m_deviceIdLabel);

    idCard->setFixedWidth(440);
    rootLayout->addWidget(idCard, 0, Qt::AlignCenter);

    rootLayout->addSpacing(28);

    // ═══════════════════════════════════════════════════════════
    // 两个按钮：透明 + 白色文字
    // ═══════════════════════════════════════════════════════════
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(28);
    btnLayout->setAlignment(Qt::AlignCenter);

    // ── 被控端按钮 ──
    m_hostBtn = new QPushButton("创建远程\n\n成为被控端，等待他人连接", this);
    m_hostBtn->setMinimumSize(240, 130);
    m_hostBtn->setCursor(Qt::PointingHandCursor);
    m_hostBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,0.12);
            color: #FFFFFF;
            border: 1.5px solid rgba(255,255,255,0.35);
            border-radius: 18px;
            font-size: 17px;
            font-weight: 700;
            padding: 24px;
        }
        QPushButton:hover {
            background: rgba(255,255,255,0.22);
            border-color: rgba(255,255,255,0.6);
        }
        QPushButton:pressed {
            background: rgba(255,255,255,0.30);
            border-color: rgba(255,255,255,0.8);
        }
    )");

    // ── 控制端按钮 ──
    m_clientBtn = new QPushButton("连接远程\n\n成为控制端，操作他人电脑", this);
    m_clientBtn->setMinimumSize(240, 130);
    m_clientBtn->setCursor(Qt::PointingHandCursor);
    m_clientBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,0.08);
            color: #FFFFFF;
            border: 1.5px solid rgba(255,255,255,0.25);
            border-radius: 18px;
            font-size: 17px;
            font-weight: 700;
            padding: 24px;
        }
        QPushButton:hover {
            background: rgba(255,255,255,0.18);
            border-color: rgba(255,255,255,0.5);
        }
        QPushButton:pressed {
            background: rgba(255,255,255,0.25);
            border-color: rgba(255,255,255,0.7);
        }
    )");

    btnLayout->addWidget(m_hostBtn);
    btnLayout->addWidget(m_clientBtn);
    rootLayout->addLayout(btnLayout);

    rootLayout->addSpacing(16);

    // ── 底部提示 ──
    auto *tip = new QLabel("同一程序，双模式切换 · 局域网 P2P 直连 · 即开即用", this);
    tip->setAlignment(Qt::AlignCenter);
    tip->setStyleSheet(
        "font-size: 12px; color: rgba(255,255,255,0.45); "
        "background: transparent;");
    rootLayout->addWidget(tip);

    rootLayout->addStretch();

    // ── 信号 ──
    connect(m_hostBtn, &QPushButton::clicked, this, &HomePage::hostModeRequested);
    connect(m_clientBtn, &QPushButton::clicked, this, &HomePage::clientModeRequested);
    connect(minBtn, &QPushButton::clicked, this, &HomePage::minimizeAppRequested);
    connect(closeBtn, &QPushButton::clicked, this, &HomePage::closeAppRequested);
}

void HomePage::setDeviceId(const QString &id)
{
    m_deviceIdLabel->setText(id);
}
