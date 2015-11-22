TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = crsync
DESTDIR = m32

SOURCES += \
    crsync.c \
    onepiece.c \
    onepiecetool.c \
    onepiece-main.c \
    ../extra/tpl.c \
    ../extra/win/mmap.c \
    log.c

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
LIBS += -L$${_PRO_FILE_PWD_}/../digest/m32 -ldigest

DEFINES += CURL_STATICLIB
INCLUDEPATH += $${_PRO_FILE_PWD_}/../libcurl/include
LIBS += -L$${_PRO_FILE_PWD_}/../libcurl/lib/m32 -lcurl -lws2_32

INCLUDEPATH += $${_PRO_FILE_PWD_}/../extra/

QMAKE_CFLAGS += -O3 -std=c99 -fopenmp
QMAKE_LFLAGS += -static -static-libgcc -fopenmp

include(deployment.pri)
qtcAddDeployment()
