### Static library - required by: movida ###

ROOT = ../..
TARGET = mvdsvgz
DESTDIR = $${ROOT}/lib
TEMPLATE = lib
QT += svg
CONFIG += plugin static
QMAKE_TARGET_DESCRIPTION = SVGZ icon engine for Movida, the free movie collection manager.

include(mvdsvgz.pri)

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}
