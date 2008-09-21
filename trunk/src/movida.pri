### Common configuration settings for all the projects that are part of movida ###

isEmpty(TARGET):error(You must set TARGET before include()'ing $${_FILE_})
isEmpty(ROOT):error(You must set ROOT before include()'ing $${_FILE_})

CONFIG += ordered debug warn_on

QMAKE_TARGET_COMPANY = Angius Fabrizio
QMAKE_TARGET_PRODUCT = Movida
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2008 Angius Fabrizio

INCLUDEPATH += $${ROOT}/src $${ROOT}/src/3rdparty $${ROOT}/src/3rdparty/iconv
DEPENDPATH += $${ROOT}/src $${ROOT}/src/3rdparty

# /lib/win32 optionally contains Windows binaries for libxml2 and other 3rd party libraries
win32 { LIBS += -L$${ROOT}/lib/win32/ }
LIBS += -L$${ROOT}/lib/

TEMP = $${ROOT}/tmp/$${TARGET}/
MOC_DIR = $${TEMP}/moc
UI_DIR = $${TEMP}/ui
OBJECTS_DIR = $${TEMP}/obj
RCC_DIR = $${TEMP}/rcc
