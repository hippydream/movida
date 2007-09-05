######### CONFIGURATION

CONFIG += ordered debug warn_on
win32-msvc {
	CONFIG += incremental
	message(win32-msvc detected. Enabling incremental build.)
}

QMAKE_TARGET_COMPANY = Angius Fabrizio
QMAKE_TARGET_PRODUCT = Movida
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2007 Angius Fabrizio


######### PATHS

INCLUDEPATH += $${ROOT}/src/3rdparty $${ROOT}/src/3rdparty/iconv
DEPENDPATH += $${ROOT}/src/3rdparty

TEMP = $${ROOT}/tmp
DESTDIR = $${ROOT}/bin

MOC_DIR = $${TEMP}/moc
UI_DIR = $${TEMP}/ui
OBJECTS_DIR = $${TEMP}/obj
RCC_DIR = $${TEMP}/rcc
