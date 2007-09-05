# Path prefix for the root of the whole project
ROOT = ../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	LIBS += $${ROOT}/lib/mvdcore.lib
} else {
	LIBS += -L$${ROOT}/lib -lmvdcore -lxml2 -lxslt
}

TARGET = mvdshared
TEMPLATE = lib
CONFIG += dll
DEFINES += MVD_BUILD_SHARED_DLL

DLLDESTDIR = $${ROOT}/bin
DESTDIR = $${ROOT}/lib

INCLUDEPATH += . ../mvdcore

QMAKE_TARGET_DESCRIPTION = Utility library for Movida, the free movie collection manager.

include(mvdshared.pri)