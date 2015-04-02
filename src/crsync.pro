TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    tpl.c \
    win/mmap.c \
    crsync.c \
    blake2b-ref.c

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    tpl.h \
    win/mman.h \
    uthash.h \
    crsync.h \
    blake2.h \
    blake2-impl.h

