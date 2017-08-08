QT += core concurrent
QT -= gui

CONFIG += c++11

TARGET = test_muduo_x_qt
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

# 3rd party lib (consider using `$$PWD` ?)
INCLUDEPATH += /home/blumia/Workspace/tmp/muduo_test/muduo-1.0.9 \
    /home/blumia/Workspace/tmp/muduo_test/simpleini-4.17/
LIBS += -L/home/blumia/Workspace/tmp/muduo_test/build/release/lib \
    -lmuduo_net -lmuduo_base -lmuduo_http

SOURCES += main.cpp \
    testmain.cpp \
    queryhandler.cpp \
    updownhandler.cpp \
    subscriberclient.cpp \
    common.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    testmain.h \
    queryhandler.h \
    updownhandler.h \
    subscriberclient.h \
    common.h
