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
} else {
	LIBS += -lxml2
}

TARGET = movidacore
TEMPLATE = lib
CONFIG += dll
DEFINES += MVD_BUILD_CORE_DLL

INCLUDEPATH += .

QMAKE_TARGET_DESCRIPTION = Core library for Movida, the free movie collection manager.

include(movidacore.pri)