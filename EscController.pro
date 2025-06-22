QT += core
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = esccontrol
TEMPLATE = app

SOURCES += \
    main.cpp \
    esccontrol.cpp

HEADERS += \
    esccontrol.h

LIBS += -lwiringPi -lpthread

DISTFILES += \
    README.md
