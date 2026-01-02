#-------------------------------------------------
# ImportaProdutos Performance Benchmark Tool
# Uses the REAL importaprodutos.cpp code
#-------------------------------------------------

QT += core sql gui widgets

TARGET = import-benchmark
TEMPLATE = app
CONFIG += console c++latest
CONFIG -= app_bundle

# Define to exclude GUI-heavy dependencies from Application class
DEFINES += BENCHMARK_BUILD

# Include QtXlsxWriter
include(../../3rdparty/QtXlsxWriter/src/xlsx/qtxlsx.pri)

# Include QSimpleUpdater (required by application.cpp)
include(../../3rdparty/QSimpleUpdater/qsimpleupdater.pri)

# Include main app source directory
INCLUDEPATH += ../../src
INCLUDEPATH += ../..  # For UI files that use "src/..." paths

# Build directories
MOC_DIR     = build/moc
OBJECTS_DIR = build/obj
RCC_DIR     = build/rcc
UI_DIR      = build/ui

# Forms from main app
FORMS += \
    ../../ui/importaprodutos.ui \
    ../../ui/validadedialog.ui

# Real source files from main app
SOURCES += \
    main.cpp \
    ../../src/importaprodutos.cpp \
    ../../src/importaprodutosproxymodel.cpp \
    ../../src/sqltablemodel.cpp \
    ../../src/sqlquery.cpp \
    ../../src/application.cpp \
    ../../src/file.cpp \
    ../../src/validadedialog.cpp \
    ../../src/dateformatdelegate.cpp \
    ../../src/doubledelegate.cpp \
    ../../src/porcentagemdelegate.cpp \
    ../../src/reaisdelegate.cpp \
    ../../src/tableview.cpp \
    ../../src/sortfilterproxymodel.cpp \
    ../../src/lineedit.cpp \
    ../../src/doublespinbox.cpp \
    ../../src/log.cpp \
    ../../src/user.cpp \
    ../../src/sqlquerymodel.cpp \
    ../../src/sqltreemodel.cpp

HEADERS += \
    ../../src/importaprodutos.h \
    ../../src/importaprodutosproxymodel.h \
    ../../src/sqltablemodel.h \
    ../../src/sqlquery.h \
    ../../src/application.h \
    ../../src/file.h \
    ../../src/validadedialog.h \
    ../../src/dateformatdelegate.h \
    ../../src/doubledelegate.h \
    ../../src/porcentagemdelegate.h \
    ../../src/reaisdelegate.h \
    ../../src/tableview.h \
    ../../src/sortfilterproxymodel.h \
    ../../src/lineedit.h \
    ../../src/doublespinbox.h \
    ../../src/log.h \
    ../../src/user.h \
    ../../src/sqlquerymodel.h \
    ../../src/sqltreemodel.h

# Platform-specific settings
win32-msvc {
    QMAKE_CXXFLAGS += /permissive-
}

win32-g++ {
    QMAKE_CXXFLAGS *= -Wall -Wextra
}

linux-g++ {
    QMAKE_CXXFLAGS *= -Wall -Wextra -Wpedantic -g -fno-omit-frame-pointer
    QMAKE_LFLAGS *= -g
}
