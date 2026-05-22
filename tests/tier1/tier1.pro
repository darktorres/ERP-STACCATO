#-------------------------------------------------
# Tier 1 — pure unit tests. No DB, no UI event loop beyond what
# QTEST_MAIN's QApplication provides. Links against libstaccato.
#-------------------------------------------------

TARGET   = tier1_tests
TEMPLATE = app
CONFIG  += console testcase c++latest warn_on
CONFIG  -= app_bundle

# Must mirror libstaccato's QT modules — the linker pulls in transitive .obj
# files (QSimpleUpdater's download_dialog.obj, LimeReport's xmlreader.obj, etc.)
# whose external refs need every module libstaccato itself uses. Including extra
# modules costs nothing at run time and saves debugging unresolved-symbol storms.
QT *= core gui sql network xml charts widgets testlib
# 3rdparty (LimeReport, QSimpleUpdater) require these too:
QT *= printsupport svg uitools qml

ROOT_PWD = $$PWD/../..

# Repo root on INCLUDEPATH for the same reason as libstaccato (ui_*.h files
# reference "src/xxx.h"). The path lookup only matters for the moc-included
# UI headers, but it's cheap to enable here.
INCLUDEPATH += $$ROOT_PWD $$ROOT_PWD/src

CONFIG(release, debug|release) {
    LIBS += -L$$ROOT_PWD/libstaccato/release -lstaccato
} else {
    LIBS += -L$$ROOT_PWD/libstaccato/debug -lstaccato
}

# OpenSSL/cURL must be on the link line too — libstaccato is an archive of
# object files; the linker pulls in whatever .obj files the test references,
# and some of those (e.g. application.cpp) transitively need OpenSSL on some
# code paths. Including the libs unconditionally avoids surprises.
win32-msvc {
    QMAKE_CXXFLAGS += /permissive-

    contains(QT_ARCH, i386) {
        LIBS += -L$$ROOT_PWD/3rdparty/OpenSSL-1.1-Win32 -llibcrypto
        LIBS += -L$$ROOT_PWD/3rdparty/cURL_x86-msvc/lib -llibcurl
    } else {
        LIBS += -L$$ROOT_PWD/3rdparty/OpenSSL-1.1-Win64 -llibcrypto
    }
}

win32-g++ {
    contains(QT_ARCH, i386) {
        LIBS += -L$$ROOT_PWD/3rdparty/OpenSSL-1.1-Win32 -llibcrypto-1_1
    } else {
        LIBS += -L$$ROOT_PWD/3rdparty/OpenSSL-1.1-Win64 -llibcrypto-1_1-x64
    }
}

linux {
    LIBS += -lcurl
}

MOC_DIR     = build_files/moc
OBJECTS_DIR = build_files/obj

SOURCES += test_tier1.cpp

# Deploy Qt + 3rdparty DLLs next to tier1_tests.exe so it can be run without
# manipulating PATH. We use the "app" profile (full deploy) because, once tests
# start referencing static methods from sql.cpp / application.cpp, the linker
# pulls in transitive .obj's that import QtCUrl symbols — making libcurl.dll
# (and friends) a load-time requirement even though the tests never call into
# that code path. The DB-safety guard at the end of main() still ensures no
# QSqlDatabase is opened at runtime.
win32-msvc {
    CONFIG(debug, debug|release): _deploy_target = debug/$${TARGET}.exe
    else: _deploy_target = release/$${TARGET}.exe

    QMAKE_POST_LINK += $$shell_path($$ROOT_PWD/tools/deploy.cmd) \
                       $$shell_quote($$shell_path($$OUT_PWD/$$_deploy_target)) app
}
