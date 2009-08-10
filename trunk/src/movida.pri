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

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../lib/
    message(macx detected. Using QMAKE_LFLAGS_SONAME.)
} else:linux-* {
    # Thanks to Paul John Floyd for suggesting this trick:
    # http://paulf.free.fr/undocumented_qmake.html
    DOLLAR = $
    QMAKE_LFLAGS += -Wl,-rpath,\'$${DOLLAR}$${DOLLAR}ORIGIN/../lib\'
    message(linux-* detected. Using QMAKE_LFLAGS.)
}

linux-g++-* {
    # Bail out on non-selfcontained libraries. Just a security measure
    # to prevent checking in code that does not compile on other platforms.
    QMAKE_LFLAGS += -Wl,--allow-shlib-undefined -Wl,--no-undefined
}

unix { PLATFORM = unix }
win32 { PLATFORM = win32 }
macx { PLATFORM = macx }

CONFIG(debug, debug|release) {
    TEMP = $${ROOT}/tmp/$${PLATFORM}/debug/$${TARGET}/
} else {
    TEMP = $${ROOT}/tmp/$${PLATFORM}/release/$${TARGET}/
}

message(Using temporary directory $${TEMP})

MOC_DIR = $${TEMP}/moc
UI_DIR = $${TEMP}/ui
OBJECTS_DIR = $${TEMP}/obj
RCC_DIR = $${TEMP}/rcc
