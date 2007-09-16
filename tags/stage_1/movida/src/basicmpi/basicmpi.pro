# Path prefix for the root of the whole project
ROOT = ../../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../src.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	LIBS += $${ROOT}/lib/win32/libxml2.lib
	LIBS += $${ROOT}/lib/win32/libxslt.lib
	LIBS += $${ROOT}/bin/movidacore.lib
	LIBS += $${ROOT}/bin/movidawidgets.lib
} else {
	LIBS += -L$${ROOT}/bin -lmovidacore -lmovidawidgets -lxml2
}

DESTDIR = $${ROOT}/bin/plugins

TARGET = basic
TEMPLATE = lib
CONFIG += dll
QT += network
DEFINES += MVD_BUILD_BASICMPI

INCLUDEPATH += . ../

QMAKE_TARGET_DESCRIPTION = Basic plugins for Movida.

include(basicmpi.pri)