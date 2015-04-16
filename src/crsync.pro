TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    tpl.c \
    crsync.c \
    blake2b-ref.c \
    win/mman.c

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    tpl.h \
    win/mman.h \
    uthash.h \
    crsync.h \
    blake2.h \
    blake2-impl.h \
    utstring.h \
    log.h

DEFINES += CURL_STATICLIB
LIBS += -L$${_PRO_FILE_PWD_}/../libcurl/lib/m32 -lcurl -lws2_32

INCLUDEPATH += $${_PRO_FILE_PWD_}/../libcurl/include

QMAKE_CFLAGS += -std=c99
QMAKE_LFLAGS += -static -static-libgcc
