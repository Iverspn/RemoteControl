#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

class QStackedWidget;
class HomePage;
class HostPage;
class ClientPage;
class RemoteSession;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUI();
    void setupConnections();

    QPixmap         m_background;
    QStackedWidget *m_pages = nullptr;
    HomePage       *m_homePage = nullptr;
    HostPage       *m_hostPage = nullptr;
    ClientPage     *m_clientPage = nullptr;
    RemoteSession  *m_session = nullptr;

    // 无边框窗口拖拽
    bool m_dragging = false;
    QPoint m_dragPos;
};

#endif // MAINWINDOW_H
