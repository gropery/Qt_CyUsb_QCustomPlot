QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    plot.cpp \
    qcustomplot/qcustomplot.cpp \
    threadinasync.cpp \
    threadinsync.cpp \
    threadloopsync.cpp \
    threadoutasync.cpp \
    threadoutsync.cpp \
    threadplotsync.cpp

HEADERS += \
    CyUsb/inc/CyAPI.h \
    mainwindow.h \
    plot.h \
    qcustomplot/qcustomplot.h \
    threadinasync.h \
    threadinsync.h \
    threadloopsync.h \
    threadoutasync.h \
    threadoutsync.h \
    threadplotsync.h

FORMS += \
    mainwindow.ui \
    plot.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/CyUsb/lib/x86/ -lCyAPI
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/CyUsb/lib/x86/ -lCyAPId

INCLUDEPATH += $$PWD/CyUsb/lib/x86
DEPENDPATH += $$PWD/CyUsb/lib/x86

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/CyUsb/lib/x86/libCyAPI.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/CyUsb/lib/x86/libCyAPId.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/CyUsb/lib/x86/CyAPI.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/CyUsb/lib/x86/CyAPId.lib

RESOURCES += \
    res.qrc

win32:CONFIG(release, debug|release): LIBS += -LC:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/ -lQt5PrintSupport
else:win32:CONFIG(debug, debug|release): LIBS += -LC:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/ -lQt5PrintSupportd
else:unix: LIBS += -LC:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/ -lQt5PrintSupport

INCLUDEPATH += C:/Qt/Qt5.14.2/5.14.2/msvc2017/include
DEPENDPATH += C:/Qt/Qt5.14.2/5.14.2/msvc2017/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += C:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/libQt5PrintSupport.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += C:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/libQt5PrintSupportd.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += C:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/Qt5PrintSupport.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += C:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/Qt5PrintSupportd.lib
else:unix: PRE_TARGETDEPS += C:/Qt/Qt5.14.2/5.14.2/msvc2017/lib/libQt5PrintSupport.a
