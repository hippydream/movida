/****************************************************************************
** Filename: preferences.cpp
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Stores application preferences at runtime.
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

#include "preferences.h"
#include "xmlpreferences.h"
#include "global.h"
#include "pathresolver.h"

#include <QString>
#include <QBitArray>
#include <QByteArray>
#include <QStringList>
#include <QDir>
#include <QColor>
#include <QFont>
#include <QApplication>
#include <QDesktopWidget>

#include <zlib/zlib.h>

#ifdef Q_WS_WIN
#include <windows.h>
#endif


/*!
	\class Preferences preferences.h
	\ingroup libmvdcore

	\brief Classes in this folder handle application global preferences.
*/

//! \internal
Preferences* Preferences::mInstance = 0;


/*!
	Returns the only allowed preferences instance.
 */
Preferences* Preferences::getInstance()
{
	if (mInstance == 0)
	{
		mInstance = new Preferences();
	}

	return mInstance;
}

/*!
	Private constructor. Init everything to default values.
 */
Preferences::Preferences()
: QObject()
{
	checkHash = writeHash = true;

	// look for correct zlib library version
	const char* my_version = ZLIB_VERSION; // ZLIB_VERSION is #defined in zlib.h
	const char* ret_version = zlibVersion();
	zlibOK = my_version[0] == ret_version[0];
	
	
	maxRecentFiles = 5;
	recentFiles = new QStringList();
	currentLang = 0;
	saveBeforeExit = true;
	confirmCloseDB = true;
	confirmDeleteMov = true;

	filterPersons = 0;
	filterCategories = 0;
	filterCountries = 0;
	filterGenres = 0;

	filterRememberPersons = true;
	filterRememberCategories = true;
	filterRememberCountries = true;
	filterRememberGenres = true;
	filterRememberMovies = true;

	quickSearchColumn = -1;

	sHistPersons = 0;
	sHistCategories = 0;
	sHistCountries = 0;
	sHistGenres = 0;
	sHistMovies = 0;

	logClearOnNewDB = false;
	logRememberLogWnd = false;
	logRememberFilter = true;

	//! \todo add truncate and initials settings to preferences dialog
	mListInitials = true;
	mListTruncate = 75;

	useLastArchiveDir = true;
	lastArchiveDir = 0;
	lastLogDir = 0;
	
	mLog = Logger::instance();
	
	mainWindowState = 0;
	startMaximized = true;
	mainWindowRect = defaultWindowRect();

	autoOrigTitle = true;
}

/*!
	Deletes the application-wide preferences instance.
	WARNING: only the main app should call this before exiting!!
 */
Preferences::~Preferences()
{
	if (this == mInstance)
	{
		delete recentFiles;
		delete currentLang;
	
		delete filterPersons;
		delete filterCategories;
		delete filterCountries;
		delete filterGenres;

		delete sHistPersons;
		delete sHistCategories;
		delete sHistCountries;
		delete sHistGenres;
		delete sHistMovies;

		delete lastArchiveDir;
		delete lastLogDir;
		
		delete mainWindowState;
	}
	else delete mInstance;
}

/*!
	Resets the preferences to their default value.
*/
void Preferences::resetPreferences()
{
	checkHash = writeHash = true;
	
	maxRecentFiles = 5;
	delete recentFiles;
	recentFiles = new QStringList();
	delete currentLang;
	currentLang = 0;
	saveBeforeExit = true;
	confirmCloseDB = true;
	confirmDeleteMov = true;

	delete filterPersons;
	filterPersons = 0;
	delete filterCategories;
	filterCategories = 0;
	delete filterCountries;
	filterCountries = 0;
	delete filterGenres;
	filterGenres = 0;

	filterRememberPersons = true;
	filterRememberCategories = true;
	filterRememberCountries = true;
	filterRememberGenres = true;
	filterRememberMovies = true;
	
	quickSearchColumn = -1;

	delete sHistPersons;
	sHistPersons = 0;
	delete sHistCategories;
	sHistCategories = 0;
	delete sHistCountries;
	sHistCountries = 0;
	delete sHistGenres;
	sHistGenres = 0;
	delete sHistMovies;
	sHistMovies = 0;

	logClearOnNewDB = false;
	logRememberLogWnd = false;
	logRememberFilter = true;

	mListInitials = true;
	mListTruncate = 75;

	useLastArchiveDir = true;
	
	delete lastArchiveDir;
	lastArchiveDir = 0;
	delete lastLogDir;
	lastLogDir = 0;
	
	delete mainWindowState;
	mainWindowState = 0;
	
	startMaximized = true;
	mainWindowRect = defaultWindowRect();

	autoOrigTitle = true;
}

/*!
	Loads the preferences from the preferences file.
*/
void Preferences::loadPreferences()
{
	XMLPreferences xml(_APP_NAME_, _COMPANY_);
	xml.setVersion(VER_MAJOR, VER_MINOR);

	PathResolver* pp = PathResolver::instance();
	const QString& fname = pp->preferencesFile();
	
	if (fname.isEmpty())
		return;
	
	if (!xml.load(fname))
		return;

	bool ok;
	int _int;
	bool _bool;
	QStringList _slist;
	QDir _dir;
	QByteArray _ba;
	QRect _rect;

	// general
	_ba = xml.getByteArray("general", "main_window_state");
	if (!_ba.isEmpty())
	{
		delete mainWindowState;
		mainWindowState = new QByteArray(_ba);
	}
	
	_bool = xml.getBool("general", "start_maximized", &ok);
	if (ok)
		startMaximized = _bool;
	
	_rect = xml.getRect("general", "main_window_rect");
	if (!_rect.isNull())
	{
		mainWindowRect = getValidWindowRect(_rect);
	}
	
	_int = xml.getInt("general", "max_recent_files", &ok);
	if (ok && (_int <= MAX_RECENT_FILES) && (_int >= MIN_RECENT_FILES))
		maxRecentFiles = _int;

	 delete recentFiles;
	 recentFiles = new QStringList(xml.getStringList("general", "recent_files"));

	 delete currentLang;
	 currentLang = new QString(xml.getString("general", "language"));

	 _bool = xml.getBool("general", "save_before_exit", &ok);
	 if (ok) saveBeforeExit = _bool;

	 _bool = xml.getBool("general", "auto_original_title", &ok);
	 if (ok) autoOrigTitle = _bool;

	// confirmations
	_bool = xml.getBool("confirmations", "close_database", &ok);
	 if (ok) confirmCloseDB = _bool;

	 _bool = xml.getBool("confirmations", "delete_movie", &ok);
	 if (ok) confirmDeleteMov = _bool;

	 // filters
	_bool = xml.getBool("filters", "remember_persons", &ok);
	if (ok) filterRememberPersons = _bool;
	_bool = xml.getBool("filters", "remember_categories", &ok);
	if (ok) filterRememberCategories = _bool;
	_bool = xml.getBool("filters", "remember_genres", &ok);
	if (ok) filterRememberGenres = _bool;
	_bool = xml.getBool("filters", "remember_movies", &ok);
	if (ok) filterRememberMovies = _bool;

	//! \todo update filters to handle history
	//! \todo quicksearch

	// log
	_bool = xml.getBool("log", "clear_on_open", &ok);
	if (ok) logClearOnNewDB = _bool;
	_bool = xml.getBool("log", "remember_log_window", &ok);
	if (ok) logRememberLogWnd = _bool;
	_bool = xml.getBool("log", "remember_filter", &ok);
	if (ok) logRememberFilter = _bool;

	// movie list
	_bool = xml.getBool("movie_list", "use_initials", &ok);
	if (ok) mListInitials = _bool;
	_int = xml.getInt("movie_list", "truncate", &ok);
	if (ok)
	{
		mListTruncate = _int < 0 ? -1
			: _int > MAX_TRUNCATE ? MAX_TRUNCATE
			: _int < MIN_TRUNCATE ? MIN_TRUNCATE : _int;
	}


	// directories
	_bool = xml.getBool("directories", "use_last_archive_dir", &ok);
	if (ok) useLastArchiveDir = _bool;
	_dir = QDir(xml.getString("directories", "archive_dir"));
	delete lastArchiveDir;
	lastArchiveDir = _dir.exists() ? new QString(_dir.absolutePath()) : 0;
	
	_dir = QDir(xml.getString("directories", "log_dir"));
	delete lastLogDir;
	lastLogDir = _dir.exists() ? new QString(_dir.absolutePath()) : 0;
}

/*!
	Saves the preferences to the the preferences file.
*/
void Preferences::savePreferences()
{
	XMLPreferences xml(_APP_NAME_, _COMPANY_);
	xml.setVersion(VER_MAJOR, VER_MINOR);
	
	PathResolver* pp = PathResolver::instance();
	const QString& fname = pp->preferencesFile();
	qDebug(fname.toAscii().data());
	
	if (fname.isEmpty())
		return;
	
	// general
	if (mainWindowState != 0)
		xml.setByteArray("general", "main_window_state", *mainWindowState, XMLPreferences::Base64);

	xml.setBool("general", "start_maximized", startMaximized);
	
	xml.setRect("general", "main_window_rect", mainWindowRect);
	
	//! \todo save columns

	xml.setBool("general", "auto_original_title", autoOrigTitle);
	
	// directories
	QDir _dir;
	QString* _str;
	Q_UNUSED(_str);
	
	xml.setBool("directories", "use_last_archive_dir", useLastArchiveDir);
	if (lastArchiveDir != 0)
	{
		_dir = QDir(*lastArchiveDir);
		if (_dir.exists()) xml.setString("directories", "archive_dir", _dir.absolutePath());
	}

	if (lastLogDir != 0)
	{
		_dir = QDir(*lastLogDir);
		if (_dir.exists()) xml.setString("directories", "log_dir", _dir.absolutePath());
	}


	//! \todo save fonts, style & more

	if (!xml.save(fname))
	{
		mLog->error(tr("Unable to save application preferences"));
	}	
}

/*!
	Returns the default window size and position.
*/
QRect Preferences::defaultWindowRect()
{
	QDesktopWidget* desktop = QApplication::desktop();
	int screenWidth = desktop->width();
	int screenHeight = desktop->height();
	
	// cannot use static fields directly inside qMin macro
	int defW = Preferences::WINDOW_WIDTH;
	int defH = Preferences::WINDOW_HEIGHT;
		
	// set width and height to the window size we want
	int width = qMin(screenWidth, defW);
	int height = qMin(screenHeight, defH);
	
	int posX = (screenWidth - width) / 2;
	int posY = (screenHeight - height) / 2;
	
	return QRect(posX, posY, width, height);
}

/*!
	Ensures the window remains inside the screen.
*/
QRect Preferences::getValidWindowRect(const QRect& r)
{
	QDesktopWidget* desktop = QApplication::desktop();
	int screenWidth = desktop->width();
	int screenHeight = desktop->height();
	
	QRect rect(r);
	
	// check window size
	if (r.width() > screenWidth)
		rect.setWidth(screenWidth);
	
	if (r.height() > screenHeight)
		rect.setHeight(screenHeight - 70);
	
	// check window position
	if (r.left() < 0)
		rect.moveLeft(0);
	else if (r.right() > screenWidth)
		rect.moveRight(screenWidth);
	
	if (r.top() < 0)
		rect.moveTop(30);
	else if (r.bottom() > screenHeight)
		rect.moveBottom(screenHeight);
	
	return rect;
}
