QT += core bluetooth
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = esccontrol
TEMPLATE = app

SOURCES += \
    esccontrolthread.cpp \
    gattserver.cpp \
    main.cpp \
    esccontrol.cpp \
    message.cpp \
    servocontroller.cpp

HEADERS += \
    esccontrol.h \
    esccontrolthread.h \
    gattserver.h \
    message.h \
    servocontroller.h

LIBS += -lwiringPi -lpthread

DISTFILES += \
    README.md

# sudo nmcli connection add type wifi con-name "SSID" ssid "SSID"
# sudo nmcli connection modify "SSID" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "password"
# sudo nmcli connection modify "SSID" connection.autoconnect yes
