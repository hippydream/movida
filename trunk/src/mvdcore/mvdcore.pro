### Shared library - required by: movida, mvdshared, mpiblue ###

ROOT = ../..
TARGET = mvdcore
DESTDIR = $${ROOT}/lib
DLLDESTDIR = $${ROOT}/bin
win32 {
	LIBS += -llibxml2 -llibxslt
} else {
	LIBS += -lxml2 -lxslt
}
TEMPLATE = lib
CONFIG += dll
DEFINES += MVD_BUILD_CORE_DLL
QMAKE_TARGET_DESCRIPTION = Core library for Movida, the free movie collection manager.

include(mvdcore.pri)

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}