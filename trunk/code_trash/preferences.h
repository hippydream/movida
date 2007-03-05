/****************************************************************************
** Filename: preferences.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Classes in this folder handle application global preferences.
** --> Stores application preferences at runtime.
** 
** Implements the Singleton design pattern for having a unique application-wide
** instance. For more info on design patterns:
** W. Gamma, R. Helm, R. Johnson, J. Vlissides & G. Booch - "Design Patterns"
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.sourceforge.net/).
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See the file LICENSE.GPL that came with this software distribution or
** visit http://www.gnu.org/copyleft/gpl.html for GPL licensing information.
**
**********************************************************************/

#ifndef MOVIDA_PREFERENCES__H
#define MOVIDA_PREFERENCES__H

#include <QHash>
#include <QByteArray>
#include <QBitArray>

#include "movidafilters.h"
#include "logger.h"

class QString;
class QStringList;

class Logger;

class Preferences : QObject
{
	Q_OBJECT
	
public:
	static Preferences* getInstance();
	~Preferences();

	void loadPreferences();
	void savePreferences();
	void resetPreferences();

	
	///////////////////////////////////////////////////////////
	// everything is public for faster access to preferences //
	///////////////////////////////////////////////////////////
	
	
	// movida version numbers
	static const int VER_MAJOR = 0;
	static const int VER_MINOR = 3;
	// lowest 8 bits are for minor version, next higher 8 bits for major
	// version 1.2 would be 0x00000102
	static const int VERSION = ((VER_MAJOR << 8) & 0xFF00) | VER_MINOR;
	
	// min and max number of recent files
	static const int MAX_RECENT_FILES = 9;
	static const int MIN_RECENT_FILES = 3;
	
	// limits for truncating long strings in a DataView
	// TODO (blue death#1#): add truncate limits handling to DataView?
	static const int MAX_TRUNCATE = 999;
	static const int MIN_TRUNCATE = 10;
	
	static const int WINDOW_WIDTH = 640;
	static const int WINDOW_HEIGHT = 480;
	
	// check or write out checksum when loading/saving Movida archives
	bool checkHash, writeHash;
	// zlib available and version compatible
	bool zlibOK;

	// maximum number of recent files visible in the MRU menu
	int maxRecentFiles;
	
	// stores recent files
	QStringList* recentFiles;
	// current application language
	QString* currentLang;
	
	// save any open and modified archive before exiting movida
	bool saveBeforeExit;

	// ask user before closing an archive
	bool confirmCloseDB;
	// ask user before deleting a movie
	bool confirmDeleteMov;

	// stores currently used filters
	PersonFilter* filterPersons;
	SimpleFilter* filterCategories;
	SimpleFilter* filterCountries;
	SimpleFilter* filterGenres;
	// TODO (blue death#1#): movie filter missing

	// remember last used filter
	bool filterRememberPersons;
	bool filterRememberCategories;
	bool filterRememberCountries;
	bool filterRememberGenres;
	bool filterRememberMovies;

	// column to use for quick search
	// TODO (blue death#1#): complete quick search feature
	int quickSearchColumn;

	// search history: remembers last used search/filter queries
	QStringList* sHistPersons;
	QStringList* sHistCategories;
	QStringList* sHistCountries;
	QStringList* sHistGenres;
	QStringList* sHistMovies;

	// clear log when opening a new archive
	bool logClearOnNewDB;
	// remember log window visibility when starting movida
	bool logRememberLogWnd;
	// remember log window filter settings
	bool logRememberFilter;

	// only display initials for person names in movie list
	bool mListInitials;
	// truncate long strings in movie list
	int mListTruncate;

	
	// remember last used directory (for movie archives)
	bool useLastArchiveDir;
	// last directory for loading/saving movida archives
	QString* lastArchiveDir;
	// always use last directory for the save log dialog!
	QString* lastLogDir;


	// currently used temp directory (i.e. for extracting MMA archives)
	QString* tempDir;
	
	// mainwindow state (dock/toolbars size and position)
	QByteArray* mainWindowState;
	bool startMaximized;
	QRect mainWindowRect;

	// true if we should automatically set the original title to the localized 
	// title in the movie editing dialog
	bool autoOrigTitle;

private:
	static Preferences* mInstance;
	Preferences();

	// movida logging interface
	Logger* mLog;
	
	// returns default main window size and position (if not maximized)
	QRect defaultWindowRect();
	// checks if window geometry in r can be used with the current screen size and
	// returns an eventually modified valid window rectangle
	QRect getValidWindowRect(const QRect& r);
};

#endif // MOVIDA_PREFERENCES__H
