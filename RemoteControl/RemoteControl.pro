QT       += core gui widgets network concurrent
CONFIG   += c++17
TARGET    = RemoteControl
TEMPLATE  = app

SOURCES += \
    main.cpp \
    src/core/RemoteSession.cpp \
    src/core/ScreenCapture.cpp \
    src/core/InputInjector.cpp \
    src/ui/MainWindow.cpp \
    src/ui/HomePage.cpp \
    src/ui/HostPage.cpp \
    src/ui/ClientPage.cpp \
    src/ui/RemoteView.cpp

HEADERS += \
    src/model/Protocol.h \
    src/core/RemoteSession.h \
    src/core/ScreenCapture.h \
    src/core/InputInjector.h \
    src/ui/MainWindow.h \
    src/ui/HomePage.h \
    src/ui/HostPage.h \
    src/ui/ClientPage.h \
    src/ui/RemoteView.h

INCLUDEPATH += src

RESOURCES += resources.qrc

# Windows: link user32 for SendInput
win32:LIBS += -luser32
