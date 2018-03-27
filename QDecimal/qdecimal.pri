INCLUDEPATH += $$PWD/decnumber
INCLUDEPATH += $$PWD/src

HEADERS += $$PWD/src/QDecContext.hh \
           $$PWD/src/QDecDouble.hh  \
           $$PWD/src/QDecPacked.hh  \
           $$PWD/src/QDecNumber.hh  \
           $$PWD/src/QDecSingle.hh  \
           $$PWD/src/QDecQuad.hh

SOURCES += $$PWD/src/QDecContext.cc \
           $$PWD/src/QDecDouble.cc  \
           $$PWD/src/QDecPacked.cc  \
           $$PWD/src/QDecNumber.cc  \
           $$PWD/src/QDecSingle.cc  \
           $$PWD/src/QDecQuad.cc

HEADERS += $$PWD/decnumber/decContext.h \
           $$PWD/decnumber/decDouble.h \
           $$PWD/decnumber/decDPD.h \
           $$PWD/decnumber/decimal128.h \
           $$PWD/decnumber/decimal32.h \
           $$PWD/decnumber/decimal64.h \
           $$PWD/decnumber/decNumber.h \
           $$PWD/decnumber/decNumberLocal.h \
           $$PWD/decnumber/decPacked.h \
           $$PWD/decnumber/decQuad.h \
           $$PWD/decnumber/decSingle.h \
           $$PWD/decnumber/decCommon.c \
           $$PWD/decnumber/decBasic.c \

SOURCES += $$PWD/decnumber/decBasic.c \
           $$PWD/decnumber/decCommon.c \
           $$PWD/decnumber/decContext.c \
           $$PWD/decnumber/decDouble.c \
           $$PWD/decnumber/decimal128.c \
           $$PWD/decnumber/decimal32.c \
           $$PWD/decnumber/decimal64.c \
           $$PWD/decnumber/decNumber.c \
           $$PWD/decnumber/decPacked.c \
           $$PWD/decnumber/decQuad.c \
           $$PWD/decnumber/decSingle.c
