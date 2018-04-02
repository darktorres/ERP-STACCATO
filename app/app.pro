#-------------------------------------------------
#
# Project created by QtCreator 2014-12-04T14:47:28
#
#-------------------------------------------------

QT       += core gui sql network xml charts uitools script printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Loja
TEMPLATE = app

CONFIG(release, debug|release){
    message(Release)
    BUILD_TYPE = release
}else{
    message(Debug)
    BUILD_TYPE = debug
}

INCLUDEPATH += ../lib/src
LIBS += -L$${OUT_PWD}/../lib/$${BUILD_TYPE} -llib

DEFINES += QT_DEPRECATED_WARNINGS

VERSION = 0.6
QMAKE_TARGET_COMPANY = Staccato Revestimentos
QMAKE_TARGET_PRODUCT = ERP
QMAKE_TARGET_DESCRIPTION = ERP da Staccato Revestimentos
QMAKE_TARGET_COPYRIGHT = Rodrigo Torres

CONFIG += c++1z

message($$QMAKESPEC)

win32-g++{
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic -Wfloat-equal -Wnarrowing
QMAKE_CXXFLAGS += -Wnull-dereference -Wold-style-cast -Wdouble-promotion -Wformat=2 -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wshadow=local

QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_CXXFLAGS_RELEASE  = -O0
QMAKE_LFLAGS_DEBUG += -O0
QMAKE_LFLAGS_RELEASE += -O0

#QMAKE_CXXFLAGS_RELEASE  = -Ofast
#QMAKE_LFLAGS_RELEASE += -O3
}

linux-g++{
    QMAKE_CC = ccache gcc
    QMAKE_CXX = ccache g++

    QMAKE_LFLAGS += -fuse-ld=gold

    #QMAKE_CXXFLAGS += -flto
    #QMAKE_LFLAGS += -flto -fuse-linker-plugin
}

linux-clang{
    QMAKE_CC = ccache clang-7
    QMAKE_CXX = ccache clang++-7

    QMAKE_LFLAGS += -fuse-ld=lld-7

    QMAKE_CXXFLAGS += -Weverything -Wno-reserved-id-macro -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-undef -Wno-padded -Wno-sign-conversion -Wno-deprecated -Wno-covered-switch-default
    QMAKE_CXXFLAGS += -Wno-undefined-reinterpret-cast -Wno-weak-vtables -Wno-exit-time-destructors -Wno-used-but-marked-unused -Wno-inconsistent-missing-destructor-override -Wno-redundant-parens
    QMAKE_CXXFLAGS += -Wno-shift-sign-overflow -Wno-non-virtual-dtor -Wno-conversion -Wno-global-constructors -Wno-switch-enum -Wno-missing-prototypes -Wno-shadow-field-in-constructor
    QMAKE_CXXFLAGS += -Wno-shadow -Wno-shadow-field

    #QMAKE_CXXFLAGS += -flto=thin
    #QMAKE_LFLAGS += -flto=thin
}

RESOURCES += \
    ../lib/qrs/resources.qrc

RC_ICONS = Staccato.ico

SOURCES += \
    main.cpp
