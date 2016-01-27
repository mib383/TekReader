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
        source/mainwindow.cpp

HEADERS  += headers/mainwindow.h \
    headers/TekVisa.h \
    headers/visa.h \
    headers/visatype.h \
    headers/vpptype.h

FORMS    += mainwindow.ui


win32: LIBS += -L$$PWD/libs/ -ltkVISA32

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

RESOURCES += \
    res.qrc

RC_FILE = rc/app.rc

DISTFILES += \
    README.md
