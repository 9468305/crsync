TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = crsync

SOURCES += \
    crsync.c \
    onepiece.c \
    onepiecetool.c \
    onepiece-main.c \
    ../extra/tpl.c \
    ../extra/win/mmap.c \
    log.c

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    crsync.h \
    crsyncver.h \
    onepiece.h \
    onepiecetool.h \
    log.h \
    ../extra/tpl.h \
    ../extra/win/mman.h \
    ../extra/uthash.h \
    ../extra/utstring.h \
    ../extra/utlist.h

INCLUDEPATH += $${_PRO_FILE_PWD_}/../digest
Debug: LIBS += -L$${_PRO_FILE_PWD_}/../digest/m32/debug -ldigest
Release: LIBS += -L$${_PRO_FILE_PWD_}/../digest/m32/release -ldigest

DEFINES += CURL_STATICLIB
LIBS += -L$${_PRO_FILE_PWD_}/../libcurl/lib/m32 -lcurl -lws2_32

INCLUDEPATH += $${_PRO_FILE_PWD_}/../libcurl/include \
               $${_PRO_FILE_PWD_}/../extra/

QMAKE_CFLAGS += -O3 -std=c99 -fopenmp
QMAKE_LFLAGS += -static -static-libgcc -fopenmp
