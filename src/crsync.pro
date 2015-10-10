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
    ../extra/blake2b-ref.c

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
    ../extra/blake2.h \
    ../extra/blake2-impl.h \
    ../extra/uthash.h \
    ../extra/utstring.h \
    ../extra/utlist.h

DEFINES += CURL_STATICLIB
LIBS += -L$${_PRO_FILE_PWD_}/../libcurl/lib/m32 -lcurl -lws2_32

INCLUDEPATH += $${_PRO_FILE_PWD_}/../libcurl/include $${_PRO_FILE_PWD_}/../extra/

QMAKE_CFLAGS += -std=c99
QMAKE_LFLAGS += -static -static-libgcc
