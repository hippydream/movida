### Shared library (mpi) - optional - requires: mvdcore, mvdshared ###

ROOT = ../..
TARGET = mpiblue
win32 {
	# Place .lib and Visual Studio crap in the bin directory and copy the dll to "Plugins"
	DESTDIR = $${ROOT}/bin
	DLLDESTDIR = "$$(APPDATA)/42cows.org/Movida/Resources/Plugins"
	message(Building plugin in \"$$DLLDESTDIR\")
}
macx {
	DESTDIR = $$(HOME)/Library/Movida/Resources/Plugins
	message(Building plugin in \"$$DESTDIR\")
}
unix {
	DESTDIR = $$(HOME)/.42cows.org/Movida/Resources/Plugins
	message(Building plugin in \"$$DESTDIR\")
}
LIBS += -lmvdcore -lmvdshared
win32 {
	LIBS += -llibxml2
} else {
	LIBS += -lxml2
}
TEMPLATE = lib
CONFIG += plugin
QT += network
DEFINES += MPI_BUILD_BLUE_DLL
QMAKE_TARGET_DESCRIPTION = Basic plugin for Movida, the free movie collection manager.

include(mpiblue.pri)

!contains(CONFIG, 'BASE_CONFIG_INCLUDED') {
	include(../movida.pri)
}