#include <QApplication>
#include <QFont>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ── 应用图标 ──
    app.setWindowIcon(QIcon(":/picture/icon.png"));

    // ── 全局字体：优先使用更美观的字体 ──
    QFont font("Microsoft YaHei", 10);
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);

    // ── 全局样式表 ──
    app.setStyleSheet(R"(
        /* ===== 滚动条 ===== */
        QScrollBar:vertical {
            background: rgba(242, 243, 245, 0.6);
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: rgba(201, 205, 212, 0.8);
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(143, 149, 158, 0.9);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }

        /* ===== 工具提示 ===== */
        QToolTip {
            background: rgba(31, 35, 41, 0.92);
            color: #FFFFFF;
            border: none;
            padding: 8px 12px;
            border-radius: 6px;
            font-size: 12px;
        }

        /* ===== 输入框通用（暗色透明） ===== */
        QLineEdit, QSpinBox {
            border: 1px solid rgba(255,255,255,0.2);
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            background: rgba(255,255,255,0.08);
            color: #FFFFFF;
        }
        QLineEdit:focus, QSpinBox:focus {
            border-color: #FF5722;
            background: rgba(255,255,255,0.14);
        }

        /* ===== 消息框 ===== */
        QMessageBox {
            background: #FFFFFF;
            border-radius: 12px;
        }
        QMessageBox QLabel {
            font-size: 13px;
            color: #1F2329;
        }
        QMessageBox QPushButton {
            min-width: 80px;
            min-height: 32px;
            border-radius: 6px;
            font-size: 13px;
            padding: 6px 20px;
        }
    )");

    MainWindow w;
    w.resize(1050, 720);
    w.setWindowTitle("RemoteControl - 远程桌面控制");
    w.show();

    return app.exec();
}
