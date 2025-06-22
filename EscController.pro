QT += core
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = esccontrol
TEMPLATE = app

SOURCES += \
    esccontrol.cpp \
    main.cpp \
    esccontrol.cpp

HEADERS += \
    esccontrol.h \
    esccontrol.h

LIBS += -lm -lwiringPi -li2c -lpthread

DISTFILES += \
    README.md
