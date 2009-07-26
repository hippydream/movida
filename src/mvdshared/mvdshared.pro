# ## Shared library - required by: movida, mpiblue - requires: mvdcore ###
ROOT = ../..
TARGET = mvdshared
DESTDIR = $${ROOT}/lib
DLLDESTDIR = $${ROOT}/bin
LIBS += -lmvdcore
win32:LIBS += -llibxml2 \
    -llibxslt
else:LIBS += -lxml2 \
    -lxslt
TEMPLATE = lib
QT += webkit
CONFIG += dll
DEFINES += MVD_BUILD_SHARED_DLL
QMAKE_TARGET_DESCRIPTION = "Utility library for Movida, the free movie collection manager."
include(mvdshared.pri)
!contains(CONFIG, 'BASE_CONFIG_INCLUDED'):include(../movida.pri)
