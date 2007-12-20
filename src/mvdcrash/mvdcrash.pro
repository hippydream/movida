### Application - optional ###

ROOT = ../..
TARGET = mvdcrash
DESTDIR = $${ROOT}/bin
LIBS += -lmvdsvgz
TEMPLATE = app
QMAKE_TARGET_DESCRIPTION = movida crash handler.

include(mvdcrash.pri)

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}