TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

TARGET = digest
DESTDIR = m32
SOURCES += md5.c blake2s-ref.c blake2b-ref.c blake2sp-ref.c blake2bp-ref.c
HEADERS += md5.h blake2.h blake2-impl.h
QMAKE_CFLAGS += -O3 -std=c99 -fopenmp
QMAKE_LFLAGS += -static -static-libgcc

include(deployment.pri)
qtcAddDeployment()
