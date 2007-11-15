# Path prefix for the root of the whole project
ROOT = ../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 { TARGET = MvdCrash }
else { TARGET = mvdcrash }
TEMPLATE = app

INCLUDEPATH += .

QMAKE_TARGET_DESCRIPTION = Movida crash handler.

include(mvdcrash.pri)
