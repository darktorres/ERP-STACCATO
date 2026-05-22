#-------------------------------------------------
# Tier 2 — integration tests against a real MySQL `staccato_test` schema.
# See .claude/test-infrastructure-plan.md (M3) and tests/README.md for the
# bootstrap details.
#-------------------------------------------------

TARGET   = tier2_tests
TEMPLATE = app
CONFIG  += console testcase c++latest warn_on
CONFIG  -= app_bundle

# Mirror libstaccato's QT modules — same rationale as tier1: the linker pulls
# in transitive .obj's that need every module libstaccato itself uses.
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

SOURCES += test_tier2.cpp \
           $$ROOT_PWD/tests/common/integration_fixture.cpp

HEADERS += $$ROOT_PWD/tests/common/integration_fixture.h

# Same "app" deploy as tier1 — the test binary inherits libstaccato's DLL
# dependencies even though it only calls into a small subset.
win32-msvc {
    CONFIG(debug, debug|release): _deploy_target = debug/$${TARGET}.exe
    else: _deploy_target = release/$${TARGET}.exe

    QMAKE_POST_LINK += $$shell_path($$ROOT_PWD/tools/deploy.cmd) \
                       $$shell_quote($$shell_path($$OUT_PWD/$$_deploy_target)) app
}
