######### CONFIGURATION

CONFIG += ordered debug warn_on

QMAKE_TARGET_COMPANY = Angius Fabrizio
QMAKE_TARGET_PRODUCT = Movida
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2007 Angius Fabrizio


######### PATHS

INCLUDEPATH += $${ROOT}/movida/src/3rdparty $${ROOT}/movida/src/3rdparty/iconv
DEPENDPATH += $${ROOT}/movida/src/3rdparty

TEMP = $${ROOT}/tmp
DESTDIR = $${ROOT}/bin

MOC_DIR = $${TEMP}/moc
UI_DIR = $${TEMP}/ui
OBJECTS_DIR = $${TEMP}/obj
RCC_DIR = $${TEMP}/rcc
