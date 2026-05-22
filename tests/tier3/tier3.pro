#-------------------------------------------------
# Tier 3 — UI smoke tests. Instantiates the real Application class and
# drives QDialog subclasses via QTest. See test_tier3.cpp for the bootstrap
# (stub lojas.txt + default QSqlDatabase pointing at staccato_test).
#-------------------------------------------------

TARGET   = tier3_tests
TEMPLATE = app
CONFIG  += console testcase c++latest warn_on
CONFIG  -= app_bundle

QT *= core gui sql network xml charts widgets testlib
QT *= printsupport svg uitools qml

ROOT_PWD = $$PWD/../..

INCLUDEPATH += $$ROOT_PWD $$ROOT_PWD/src $$ROOT_PWD/tests/common

CONFIG(release, debug|release) {
    LIBS += -L$$ROOT_PWD/libstaccato/release -lstaccato
} else {
    LIBS += -L$$ROOT_PWD/libstaccato/debug -lstaccato
}

win32-msvc {
    QMAKE_CXXFLAGS += /permissive-

    contains(QT_ARCH, i386) {
        LIBS += -L$$ROOT_PWD/3rdparty/OpenSSL-1.1-Win32 -llibcrypto
        LIBS += -L$$ROOT_PWD/3rdparty/cURL_x86-msvc/lib -llibcurl
    } else {
        LIBS += -L$$ROOT_PWD/3rdparty/OpenSSL-1.1-Win64 -llibcrypto
    }
}

linux {
    LIBS += -lcurl
}

MOC_DIR     = build_files/moc
OBJECTS_DIR = build_files/obj

SOURCES += test_tier3.cpp \
           $$ROOT_PWD/tests/common/integration_fixture.cpp

HEADERS += $$ROOT_PWD/tests/common/integration_fixture.h

# Tier 3 instantiates Application — needs the full app DLL deploy.
win32-msvc {
    CONFIG(debug, debug|release): _deploy_target = debug/$${TARGET}.exe
    else: _deploy_target = release/$${TARGET}.exe

    QMAKE_POST_LINK += $$shell_path($$ROOT_PWD/tools/deploy.cmd) \
                       $$shell_quote($$shell_path($$OUT_PWD/$$_deploy_target)) app
}
