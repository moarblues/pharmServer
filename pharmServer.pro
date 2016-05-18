#-------------------------------------------------
#
# Project created by QtCreator 2013-09-03T01:35:48
#
#-------------------------------------------------

QT       += core gui network sql

CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pharmServer
TEMPLATE = app

DESTDIR = bin

SOURCES += main.cpp\
        srvmainwindow.cpp \
    server.cpp \
    client.cpp \
    database.cpp \
    files.cpp \

HEADERS  += srvmainwindow.h \
    server.h \
    client.h \
    database.h \
    files.h \

FORMS    += srvmainwindow.ui
