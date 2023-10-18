QT += bluetooth

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32:RC_ICONS += $$\PWD\icons\esc.png

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

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle.properties \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/drawable-xhdpi/icon.png \
    android/res/drawable-xxhdpi/icon.png \
    android/res/drawable-xxxhdpi/icon.png \
    android/res/values/libs.xml



