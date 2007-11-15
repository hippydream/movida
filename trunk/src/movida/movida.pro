# Path prefix for the root of the whole project
ROOT = ../..

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}

message(Using temporary directory \"$${TEMP}\")

win32 {
	message(Using lib directory \"$${ROOT}/lib/win32\")
	LIBS += $${ROOT}/lib/win32/mvdcore.lib $${ROOT}/lib/win32/mvdshared.lib
} else {
	LIBS += -L$${ROOT}/lib -lmvdcore -lmvdshared -lxml2 -lxslt
}

win32 { TARGET = Movida }
else { TARGET = movida }
TEMPLATE = app

INCLUDEPATH += . ../mvdcore ../mvdshared

QMAKE_TARGET_DESCRIPTION = Movida, the free movie collection manager.

include(movida.pri)
