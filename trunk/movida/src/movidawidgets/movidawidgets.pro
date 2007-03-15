# Path prefix for the root of the whole project
ROOT = ../../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../src.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	#LIBS += $${ROOT}/lib/win32/libxml2.lib
	#LIBS += $${ROOT}/lib/win32/libxslt.lib
	LIBS += $${ROOT}/bin/movidacore.lib
} else {
	LIBS += -L$${ROOT}/bin -lmovidacore
}

TARGET = movidawidgets
TEMPLATE = lib
CONFIG += dll
DEFINES += MVD_BUILD_MOVIDAWIDGETS

INCLUDEPATH += .

QMAKE_TARGET_DESCRIPTION = Shared widgets for Movida.

include(movidawidgets.pri)