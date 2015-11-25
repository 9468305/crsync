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
    helper.c \
    util.c \
    log.c \
    onepiece.c \
    onepiecetool.c \
    onepiece-main.c \
    ../extra/tpl.c \
    ../extra/win/mmap.c \
    ../extra/dictionary.c \
    ../extra/iniparser.c

HEADERS += \
    digest.h \
    diff.h \
    patch.h \
    http.h \
    crsync.h \
    helper.h \
    util.h \
    log.h \
    define.h \
    crsyncver.h \
    onepiece.h \
    onepiecetool.h \
    ../extra/tpl.h \
    ../extra/win/mman.h \
    ../extra/uthash.h \
    ../extra/utstring.h \
    ../extra/utlist.h \
    ../extra/dictionary.h \
    ../extra/iniparser.h

INCLUDEPATH += $${_PRO_FILE_PWD_}/../digest
LIBS += -L$${_PRO_FILE_PWD_}/../digest/m32 -ldigest

DEFINES += CURL_STATICLIB
INCLUDEPATH += $${_PRO_FILE_PWD_}/../libcurl/include
LIBS += -L$${_PRO_FILE_PWD_}/../libcurl/lib/m32 -lcurl -lws2_32

INCLUDEPATH += $${_PRO_FILE_PWD_}/../extra/

QMAKE_CFLAGS += -O3 -std=c99 -fopenmp
QMAKE_LFLAGS += -static -static-libgcc -fopenmp

win32:RC_FILE = crsync.rc

include(deployment.pri)
qtcAddDeployment()
