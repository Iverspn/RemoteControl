#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);

    void setDeviceId(const QString &id);

signals:
    void hostModeRequested();
    void clientModeRequested();
    void minimizeAppRequested();
    void closeAppRequested();

private:
    QLabel      *m_deviceIdLabel  = nullptr;
    QPushButton *m_hostBtn        = nullptr;
    QPushButton *m_clientBtn      = nullptr;
};

#endif // HOMEPAGE_H
