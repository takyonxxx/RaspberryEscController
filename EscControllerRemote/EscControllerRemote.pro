QT += bluetooth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32:RC_ICONS += $$\PWD\icons\robot.png

QT_EVENT_DISPATCHER_CORE_FOUNDATION=1

TARGET = EscControllerRemote
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
        deviceinfo.cpp \
        message.cpp \
        bluetoothclient.cpp

HEADERS  += mainwindow.h \
        deviceinfo.h \
        message.h \
        bluetoothclient.h

FORMS    += mainwindow.ui

INCLUDEPATH += .

RESOURCES += \
    resources.qrc

ios: QMAKE_INFO_PLIST = ./ios/Info.plist
macos: QMAKE_INFO_PLIST = ./macos/Info.plist

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}



