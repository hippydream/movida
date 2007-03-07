# Path prefix for the root of the whole project
ROOT = ../../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../src.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	LIBS += $${ROOT}/bin/movidacore.lib
} else {
	LIBS += -L$${ROOT}/bin -lmovidacore -lxml2 -lxslt
}

win32 { TARGET = Movida }
else { TARGET = movida}
TEMPLATE = app

INCLUDEPATH += . ../movidacore

QMAKE_TARGET_DESCRIPTION = Movida, the free movie collection manager.

include(movida.pri)
