#### ABOUT ####
#


#### CONFIG ####

PROJECT = imdbproc
ROOT = ../..
CODEPATH = .

TEMP = $${ROOT}/tmp/$${PROJECT}

message(Building project \"$${PROJECT}\")
message(Using temporary directory \"$${TEMP}\")
message(Using code path \"$${CODEPATH}\")

win32 {
#LIBS += $${ROOT}/lib/win32/libxml2.lib
}
else {
#LIBS += -lxml2
}

TARGET = imdbproc
CONFIG += debug warn_on
TEMPLATE = app

DESTDIR = $${ROOT}/bin

MOC_DIR = $${TEMP}/moc
UI_DIR = $${TEMP}/ui
OBJECTS_DIR = $${TEMP}/obj
RCC_DIR = $${TEMP}/rcc

INCLUDEPATH = $${CODEPATH} $${CODEPATH}/include $${CODEPATH}/include/3rdparty
DEPENDPATH = $${CODEPATH} $${CODEPATH}/include $${CODEPATH}/include/3rdparty

#### INCLUDE FILE(S) ####
include(imdbproc.pri)
