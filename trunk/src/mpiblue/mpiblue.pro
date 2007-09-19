# Path prefix for the root of the whole project
ROOT = ../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	LIBS += $${ROOT}/lib/win32/mvdcore.lib
	LIBS += $${ROOT}/lib/win32/mvdshared.lib
	LIBS += $${ROOT}/lib/win32/libxml2.lib
} else {
	LIBS += -L$${ROOT}/lib -lmvdcore -lmvdshared -lxml2
}

TARGET = mpiblue
TEMPLATE = lib
CONFIG += dll
DEFINES += MPI_BUILD_BLUE_DLL
QT += network

DLLDESTDIR = $${ROOT}/bin/plugins
win32 {
	DESTDIR = $${ROOT}/lib/win32
} else {
	DESTDIR = $${ROOT}/lib
}

INCLUDEPATH += . ../mvdcore ../mvdshared

QMAKE_TARGET_DESCRIPTION = Basic plugin for Movida, the free movie collection manager.

include(mpiblue.pri)