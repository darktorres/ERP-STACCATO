#-------------------------------------------------
# Thin app shell. All sources except main.cpp live in
# libstaccato/libstaccato.pro (see .claude/test-infrastructure-plan.md M1).
#-------------------------------------------------

!versionAtLeast(QT_VERSION, 5.15.0) {
    error("Use Qt 5.15 ou mais novo")
}

TARGET = Loja
TEMPLATE = app

QT *= core gui sql network xml charts widgets

DEFINES *= QT_DEPRECATED_WARNINGS

CONFIG *= c++latest warn_on

win32 {
    VERSION = 0.10.153
    QMAKE_TARGET_COMPANY = Staccato Revestimentos
    QMAKE_TARGET_PRODUCT = ERP
    QMAKE_TARGET_DESCRIPTION = ERP da Staccato Revestimentos
    QMAKE_TARGET_COPYRIGHT = Rodrigo Torres

    RC_ICONS = Staccato.ico
}

win32-msvc {
    QMAKE_CXXFLAGS += /permissive-

    contains(QT_ARCH, i386) {
        LIBS += -L$$_PRO_FILE_PWD_/3rdparty/OpenSSL-1.1-Win32 -llibcrypto
        LIBS += -L$$_PRO_FILE_PWD_/3rdparty/cURL_x86-msvc/lib -llibcurl
    } else {
        LIBS += -L$$_PRO_FILE_PWD_/3rdparty/OpenSSL-1.1-Win64 -llibcrypto
    }
}

win32-g++ {
    contains(QT_ARCH, i386) {
        LIBS += -L$$_PRO_FILE_PWD_/3rdparty/OpenSSL-1.1-Win32 -llibcrypto-1_1
    } else {
        LIBS += -L$$_PRO_FILE_PWD_/3rdparty/OpenSSL-1.1-Win64 -llibcrypto-1_1-x64
    }
}

*-g++ {
    QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
    QMAKE_CXXFLAGS *= -Wall -Wextra -Wpedantic
}

*-clang {
    QMAKE_CXXFLAGS *= -Wno-deprecated-enum-enum-conversion
}

linux-g++ {
    QMAKE_LFLAGS *= -fuse-ld=gold
}

linux-clang {
    QMAKE_LFLAGS *= -fuse-ld=lld-13
}

linux {
    LIBS += -lcurl
}

#-----------------------------------------------------

MOC_DIR        = build_files/moc
UI_DIR         = build_files/ui
UI_HEADERS_DIR = build_files/ui
UI_SOURCES_DIR = build_files/ui
OBJECTS_DIR    = build_files/obj
RCC_DIR        = build_files/rcc

#-----------------------------------------------------
# Link against libstaccato (static lib built by libstaccato/libstaccato.pro)

INCLUDEPATH += $$PWD/src

CONFIG(release, debug|release) {
    LIBS += -L$$PWD/libstaccato/release -lstaccato
    win32-msvc: PRE_TARGETDEPS += $$PWD/libstaccato/release/staccato.lib
} else {
    LIBS += -L$$PWD/libstaccato/debug -lstaccato
    win32-msvc: PRE_TARGETDEPS += $$PWD/libstaccato/debug/staccato.lib
}

#-----------------------------------------------------
# Re-include 3rdparty .pri files *only* to harvest their RESOURCES. Static-lib
# .qrc on MSVC needs Q_INIT_RESOURCE() in main() to auto-init; placing the
# .qrc compilation in the exe avoids that. SOURCES/HEADERS/FORMS get cleared
# so we don't double-compile them (they live in libstaccato).

include(3rdparty/QtXlsxWriter/src/xlsx/qtxlsx.pri)
include(3rdparty/QSimpleUpdater/qsimpleupdater.pri)
versionAtMost(QT_VERSION, 5.15.2) { include(3rdparty/LimeReport-1.5.68/limereport/limereport.pri) }
SOURCES =
HEADERS =
FORMS =

#-----------------------------------------------------

RESOURCES += qrs/resources.qrc

SOURCES += src/main.cpp

#-----------------------------------------------------
# Deploy runtime dependencies next to Loja.exe after each link.
# tools/deploy.cmd runs windeployqt and copies 3rdparty DLLs. Config files
# (mysql.txt, lojas.txt, google_api.txt) hold secrets and are NOT deployed —
# provide them manually in the .exe's working directory.

win32-msvc {
    CONFIG(debug, debug|release): _deploy_target = debug/$${TARGET}.exe
    else: _deploy_target = release/$${TARGET}.exe

    QMAKE_POST_LINK += $$shell_path($$PWD/tools/deploy.cmd) \
                       $$shell_quote($$shell_path($$OUT_PWD/$$_deploy_target)) app
}
