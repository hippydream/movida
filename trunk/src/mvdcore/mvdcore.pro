# Path prefix for the root of the whole project
ROOT = ../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	LIBS += $${ROOT}/lib/win32/libxml2.lib
	LIBS += $${ROOT}/lib/win32/libxslt.lib
} else {
	LIBS += -lxml2
}

TARGET = mvdcore
TEMPLATE = lib
CONFIG += dll
DEFINES += MVD_BUILD_CORE_DLL

DLLDESTDIR = $${ROOT}/bin
win32 {
	DESTDIR = $${ROOT}/lib/win32
} else {
	DESTDIR = $${ROOT}/lib
}

INCLUDEPATH += .

QMAKE_TARGET_DESCRIPTION = Core library for Movida, the free movie collection manager.

include(mvdcore.pri)