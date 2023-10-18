TEMPLATE = app
QT -= gui
QT += bluetooth

CONFIG += c++17 console
CONFIG -= app_bundle
TARGET = EscController

SOURCES += main.cpp \
    esccontroller.cpp \
    i2cdev.cpp \
    message.cpp \
    mpu6050.cpp \
    pid.cpp \
    robotcontrol.cpp \
    pwmcontrol.cpp \
    gattserver.cpp

HEADERS += esccontroller.h \
    constants.h \    
    i2cdev.h \
    kalman.h \   
    message.h \
    mpu6050.h \
    pid.h \
    robotcontrol.h \
    pwmcontrol.h \
    gattserver.h

QMAKE_INCDIR += /usr/local/include
QMAKE_LIBDIR += /usr/lib
QMAKE_LIBDIR += /usr/local/lib
QMAKE_LIBDIR += /usr/lib/x86_64-linux-gnu
INCLUDEPATH += /usr/local/include

LIBS += -lm -lwiringPi -li2c
