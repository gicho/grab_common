
HEADERS += \
    $$PWD/inc/rotations.h \
    $$PWD/inc/quaternions.h

SOURCES += \
    $$PWD/src/rotations.cpp \
    $$PWD/src/quaternions.cpp

INCLUDEPATH += $$PWD/inc

QT       -= gui

CONFIG   += staticlib c++11
CONFIG   -= app_bundle

TEMPLATE = lib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# Lib numeric
unix:!macx: LIBS += -L$$PWD/../libnumeric/lib/ -lnumeric

INCLUDEPATH += $$PWD/../libnumeric $$PWD/../libnumeric/inc
DEPENDPATH += $$PWD/../libnumeric

unix:!macx: PRE_TARGETDEPS += $$PWD/../libnumeric/lib/libnumeric.a
