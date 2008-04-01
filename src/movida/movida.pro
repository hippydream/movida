### Application - requires: mvdcore, mvdshared, mvdsvgz ###

ROOT = ../..
TARGET = movida
DESTDIR = $${ROOT}/bin
LIBS += -lmvdcore -lmvdsvgz -lmvdshared
win32 {
	LIBS += -llibxml2 -llibxslt
} else {
	LIBS += -lxml2 -lxslt
}
TEMPLATE = app
QT += svg network
QMAKE_TARGET_DESCRIPTION = Movida, the free movie collection manager.

include(movida.pri)

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}