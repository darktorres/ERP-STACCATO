SOURCES += \ 
    tst_tests.cpp

QT += testlib
QT += core gui sql network xml charts uitools script printsupport
CONFIG += qt warn_on depend_includepath testcase

CONFIG += c++1z

CONFIG(release, debug|release){
    message(Release)
    BUILD_TYPE = release
}else{
    message(Debug)
    BUILD_TYPE = debug
}

INCLUDEPATH += ../lib/src
LIBS += -L$${OUT_PWD}/../lib/$${BUILD_TYPE} -llib
