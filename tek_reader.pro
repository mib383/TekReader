#-------------------------------------------------
#
# Project created by QtCreator 2015-10-30T17:09:14
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tek_reader
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    TekVisa.h \
    visa.h \
    visatype.h \
    vpptype.h

FORMS    += mainwindow.ui


win32: LIBS += -L$$PWD/ -ltkVISA32

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

RESOURCES += \
    res.qrc

RC_FILE = app.rc
