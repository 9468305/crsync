TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = crsync
DESTDIR = m32

SOURCES += \
    digest.c \
    diff.c \
    patch.c \
    http.c \
    crsync.c \
    magnet.c \
    helper.c \
    util.c \
    log.c \
    onepiece-main.c \
    md5.c \
    ../extra/tpl.c \
    ../extra/win/mmap.c \
    ../extra/dictionary.c \
    ../extra/iniparser.c

HEADERS += \
    global.h \
    digest.h \
    diff.h \
    patch.h \
    http.h \
    crsync.h \
    magnet.h \
    helper.h \
    util.h \
    log.h \
    crsyncver.h \
    md5.h \
    ../extra/tpl.h \
    ../extra/win/mman.h \
    ../extra/uthash.h \
    ../extra/utstring.h \
    ../extra/utlist.h \
    ../extra/dictionary.h \
    ../extra/iniparser.h

DEFINES += CURL_STATICLIB
INCLUDEPATH += $${_PRO_FILE_PWD_}/../libcurl/include
LIBS += -L$${_PRO_FILE_PWD_}/../libcurl/lib/m32 -lcurl -lws2_32

INCLUDEPATH += $${_PRO_FILE_PWD_}/../extra/

QMAKE_CFLAGS += -O3 -std=c99 -fopenmp
QMAKE_LFLAGS += -static -static-libgcc -fopenmp

win32:RC_FILE = crsync.rc

include(deployment.pri)
qtcAddDeployment()
