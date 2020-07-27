#
#  This file is part of QSimpleUpdater
#
#  Copyright (c) 2014 Alex Spataru <alex_spataru@outlook.com>
#
#  Please check the license.txt file for more information.
#

QT *= gui widgets network

HEADERS += $$PWD/src/qsimpleupdater.h \
           $$PWD/src/dialogs/download_dialog.h \
           $$PWD/src/dialogs/progress_dialog.h

SOURCES += $$PWD/src/qsimpleupdater.cpp \
           $$PWD/src/dialogs/download_dialog.cpp \
           $$PWD/src/dialogs/progress_dialog.cpp

INCLUDEPATH += $$PWD/src

RESOURCES += $$PWD/res/qsu_resources.qrc

FORMS += $$PWD/src/dialogs/download_dialog.ui \
         $$PWD/src/dialogs/progress_dialog.ui
