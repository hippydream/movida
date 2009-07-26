/**************************************************************************
** Filename: pathresolver.cpp
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
**
** This file is part of the Movida project (http://movida.42cows.org/).
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
**************************************************************************/

#include "pathresolver.h"
#include "global.h"
#include "core.h"
#include <QDir>
#include <QString>
#include <QCoreApplication>
#include <QtGlobal>
#include <QUuid>
#include <QDateTime>
#include <QMutex>
#include <stdexcept>

#ifdef Q_WS_WIN
#include "qt_windows.h"
#include "qlibrary.h"
#ifndef CSIDL_COMMON_APPDATA
#define CSIDL_COMMON_APPDATA 0x0023 // All Users\Application Data
#endif
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a // <username>\Application Data
#endif
#else
#include <unistd.h>
#endif
#include <time.h>

// We cannot use the logger here because this class needs to compute the log file path first!!
#include <QtDebug>

using namespace Movida;

Q_GLOBAL_STATIC(QMutex, MvdPathResolverLock)

/*!
	\class MvdPathResolver pathresolver.h
	\ingroup MvdCore Singletons

	\brief Handles platform-specific directory or file paths.
	The returned paths always end with a directory separator.

	<b>Movida::paths()</b> can be used as a convenience method to access the singleton.

	Please refer to the individual methods for the location of the path on
	each platform.
	
	Values between curly brackets (i.e. "{PATH}") refer to environment variables, with the following
	exceptions:
	\verbatim
		{Org} refers to QCoreApplication::organizationName
		{App} refers to QCoreApplication::applicationName
		{Dom} refers to QCoreApplication::organizationDomain
		{AppDir} refers to QCoreApplication::applicationDirPath()
		{Pid} refers to the process ID
		{Time_t} refers to the current time in time_t format (i.e. the number of seconds since 1970-01-01T00:00:00, UTC).
		{Idx} refers to a unique numeric ID
		{Bundle} refers to the path of the application bundle on Mac OS X (i.e. /Applications/Movida.app)
	\endverbatim

	Values between double curly brackets (i.e. "{{SSF\Common AppData}}" refer to Windows registry entries,
	with the following abbreviations being used:
	\verbatim
		SSF = "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders\"
		USF = "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders\"
	\endverbatim

	\warning
	Please ensure that QCoreApplication::setApplicationName(), 
	QCoreApplication::setOrganizationName() and QCoreApplication::setOrganizationDomain()
	have been called before using this singleton or the wrong directories will be initialized!
*/


/************************************************************************
MvdPathResolver_P
*************************************************************************/

class MvdPathResolver::Private
{
public:
	Private();

	bool initialized;

	QString settingsDir;
	QString settingsFile;
	QString logFile;
	QString tempDir;
	QString appDir;
	QString resourcesDirUser;
	QString resourcesDirSystem;

#ifdef Q_WS_WIN
	QString specialFolder(int type);
	bool isNtBased;
#endif
};

//! \internal Initializes paths.
MvdPathResolver::Private::Private()
{
	initialized = false;
#ifdef Q_WS_WIN
	isNtBased = (bool)(QSysInfo::WindowsVersion & QSysInfo::WV_NT_based);
#endif

	QString org = QCoreApplication::organizationName();
	QString app = QCoreApplication::applicationName();
	QString dom = QCoreApplication::organizationDomain();

	//// 0. settings dir
#if defined(Q_WS_WIN)
	if (isNtBased)
	{
		QString appData = specialFolder(CSIDL_APPDATA);
		QDir dir(appData);
		appData = dir.absolutePath();
		if (!dir.exists() && !dir.mkpath(appData))
			qDebug() << "MvdPathResolver: Failed to create/locate directory" << appData;
		else
		{
			appData = MvdCore::toLocalFilePath(appData, true);
			this->settingsDir = appData;
		}
	}
	if (this->settingsDir.isEmpty())
	{
		QString root = QCoreApplication::applicationDirPath();
		QDir dir(root);
		root = dir.absolutePath();
		if (!dir.exists())
		{
			qDebug() << "MvdPathResolver: failed to locate directory" << root;
			return;
		}
		root = MvdCore::toLocalFilePath(root, true);
		this->settingsDir = root;
	}
#else
	QString root = QDir::homePath();
	QDir dir(root);
	root = dir.absolutePath();
	if (!dir.exists())
	{
		qDebug() << "MvdPathResolver: failed to locate directory" << root;
		return;
	}
	root = MvdCore::toLocalFilePath(root, true);
	this->settingsDir = root;
#endif

#if defined(Q_WS_WIN)
	this->settingsFile = QString(this->settingsDir).append(org).append(QDir::separator()).append(app).append(".xml");
#else
	// Hack for QSettings. See settings.cpp file.
	this->settingsFile = QString(this->settingsDir).append(".").append(org).append(QDir::separator()).append(app).append(".xml");
#endif

	qDebug() << "MvdPathResolver: storing application settings in" << this->settingsFile << ".";


	//// 1. temp dir
	QDateTime dt = QDateTime::currentDateTime();
	QString tempDirPath = QDir::tempPath().append("/").append(org).append("/").append(app).append(".")
		.append(QString::number(dt.toTime_t()));
	QDir tempDir(tempDirPath);
	tempDirPath = tempDir.absolutePath();
	if (tempDir.exists()) {
		bool failed = false;
		QFileInfo info(tempDirPath);
		if (info.isDir())
			failed = !tempDir.rmpath(tempDirPath);
		else failed = !QFile::remove(tempDirPath);
		if (failed)
		{
			qDebug() << "MvdPathResolver: Failed to create a temporary directory" << tempDirPath;
			return; // initialized is false and the app is supposed to terminate
		}
	}
	
	tempDir.mkpath(tempDirPath);
	if (!tempDir.exists())
	{
		qDebug() << "MvdPathResolver: Failed to create a temporary directory" << tempDirPath;
		return; // initialized is false and the app is supposed to terminate
	}

	this->tempDir = MvdCore::toLocalFilePath(tempDirPath, true);
	qDebug() << "MvdPathResolver: using" << this->tempDir << "as temporary directory.";

	
	//// 2. resources dir

#if defined(Q_WS_WIN)
	bool resourcesOk = false;
	if (isNtBased)
	{
		for (int i = 0; i < 2; ++i) {
			QString root = i == 0 ? specialFolder(CSIDL_APPDATA) : specialFolder(CSIDL_COMMON_APPDATA);
			if (root.isEmpty())
				continue;
			root.append("/").append(org).append("/").append(app).append("/Resources");
			QDir dir(root);
			root = dir.absolutePath();
			if (!dir.exists() && !dir.mkpath(root))
			{
				qDebug() << "MvdPathResolver: failed to create/locate directory" << root;
				continue;
			}
			root = MvdCore::toLocalFilePath(root, true);
			if (i == 0)
				this->resourcesDirUser = root;
			else
				this->resourcesDirSystem = root;
		}
		resourcesOk = !this->resourcesDirUser.isEmpty();
	}

	if (!resourcesOk)
	{
		// Either Win9x or some problem occurred. Fall back to "{AppDir}\Resources\".
		QString root = QCoreApplication::applicationDirPath();
		root.append("/Resources");
		QDir dir(root);
		root = dir.absolutePath();
		if (!dir.exists() && !dir.mkpath(root))
		{
			qDebug() << "MvdPathResolver: failed to create/locate directory" << root;
			return;
		}
		root = MvdCore::toLocalFilePath(root, true);
		this->resourcesDirUser = root;
	}
#endif
#if defined(Q_WS_MAC)
	QString userResources = QDir::homePath().append("/Library/").append(app).append("/Resources");
	QDir dir(userResources);
	userResources = dir.absolutePath();
	if (!QDir::exists(userResources) && !dir.mkpath(userResources))
	{
		qDebug() << "MvdPathResolver: failed to create/locate directory" << userResources;
		return;
	}
	
	this->resourcesDirUser = MvdCore::toLocalFilePath(userResources, true);

	QString sysResources = QCoreApplication::applicationDirPath().append("/../Resources");
	sysResources = QDir::cleanPath(sysResources);
	QDir sysResourcesDir(sysResources);
	sysResources = sysResourcesDir.absolutePath();
	if (!sysResourcesDir.exists())
	{
		qDebug() << "MvdPathResolver: failed to locate directory" << sysResources;
	}
	else
	{
		sysResources = MvdCore::toLocalFilePath(sysResources, true);
		this->resourcesDirSystem = sysResources;
	}
#endif
#if defined(Q_WS_X11)
	QString userResources = QDir::homePath().append("/.").append(org).append("/").append(app).append("/Resources");
	dir = QDir(userResources);
	userResources = dir.absolutePath();
	if (!dir.exists() && !dir.mkpath(userResources))
	{
		qDebug() << "MvdPathResolver: failed to create/locate directory" << userResources;
		return;
	}
	this->resourcesDirUser = MvdCore::toLocalFilePath(userResources, true);

	QString sysResources = QString("/etc/").append(org).append("/").append(app).append("/Resources");
	dir = QDir(sysResources);
	sysResources = dir.absolutePath();
	if (!dir.exists() && !dir.mkpath(sysResources))
		qDebug() << "MvdPathResolver: failed to create/locate directory" << sysResources;
	else
	{
		sysResources = MvdCore::toLocalFilePath(sysResources, true);
		this->resourcesDirSystem = sysResources;
	}
#endif

	qDebug() << "MvdPathResolver: using" << this->resourcesDirUser << "as user resources directory.";
	qDebug() << "MvdPathResolver: using" << this->resourcesDirSystem << "as system resources directory.";


	//// 3. log file

	QString log = this->resourcesDirUser;
	log.append("MovidaLog.html");
	qDebug() << "MvdPathResolver: writing log to" << log << ".";
	this->logFile = log;

	initialized = true;
}

#ifdef Q_WS_WIN

/*!
	\internal Returns the path to a system shell folder using SHGetSpecialFolderPathW or
	SHGetSpecialFolderPathA from shell32.dll.
*/
QString MvdPathResolver::Private::specialFolder(int type)
{
	QString result;

	QLibrary library(QLatin1String("shell32"));
	QT_WA( {
		typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPTSTR, int, BOOL);
		GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathW");
		if (SHGetSpecialFolderPath) {
			TCHAR path[MAX_PATH];
			SHGetSpecialFolderPath(0, path, type, FALSE);
			result = QString::fromUtf16((ushort*)path);
		}
	} , {
		typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, char*, int, BOOL);
		GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathA");
		if (SHGetSpecialFolderPath) {
			char path[MAX_PATH];
			SHGetSpecialFolderPath(0, path, type, FALSE);
			result = QString::fromLocal8Bit(path);
		}
	} );

	if (result.isEmpty()) {
		switch (type) {
		case CSIDL_COMMON_APPDATA:
			result = QLatin1String("C:\\temp\\qt-common");
			break;
		case CSIDL_APPDATA:
			result = QLatin1String("C:\\temp\\qt-user");
			break;
		default:
			;
		}
	}

	return result;
}

#endif


/************************************************************************
MvdPathResolver
*************************************************************************/

//! \internal
volatile MvdPathResolver* MvdPathResolver::mInstance = 0;
bool MvdPathResolver::mDestroyed = false;

//!	\internal Private constructor - initialize paths.
MvdPathResolver::MvdPathResolver()
: d(new Private)
{
}

//! Returns the unique application instance.
MvdPathResolver& MvdPathResolver::instance()
{
	if (!mInstance) {
		QMutexLocker locker(MvdPathResolverLock());
		if (!mInstance) {
			if (mDestroyed)
				throw std::runtime_error("Pathresolver: access to dead reference");
			create();
		}
	}

	return (MvdPathResolver&) *mInstance;
}

//! Destructor.
MvdPathResolver::~MvdPathResolver()
{
	delete d;
	mInstance = 0;
	mDestroyed = true;
}

void MvdPathResolver::create()
{
	// Local static members are instantiated as soon 
	// as this function is entered for the first time
	// (Scott Meyers singleton)
	static MvdPathResolver instance;
	mInstance = &instance;
}

//! Returns false if some required directory or file could not be created.
bool MvdPathResolver::isInitialized() const
{
	return d->initialized;
}

/*!
	Returns the absolute path of a directory that contains the user specific settings
	file. Please append "/{Org}/{App}.xml" to the returned value to have the exact
	filename (this is necessary because of the current implementation of MvdSettings).

	<table>
		<tr><th>Platform</th><th>Scope</th><th>Location</th><th>Notes</th></tr>
		<tr><td>Windows NT</td><td></td><td>{{USF\AppData}}\{Org}\{App}\</td><td></td>
		<tr><td>Windows 9x</td><td></td><td>{AppDir}\</td><td></td>
		<tr><td>Unix / Mac OS X</td><td></td><td>{HOME}/.config/{Org}/{App}/</td><td></td>
	</table>
*/
QString MvdPathResolver::settingsDir() const
{
	return d->settingsDir;
}

/*!
	Convenience method, returns the absolute path to the settings file.
*/
QString MvdPathResolver::settingsFile() const
{
	return d->settingsFile;
}

/*!
	Returns the full path to the application log file.
	The file is placed in the user's resources directory, so please refer to
	MvdPathResolver::resourcesDir() for details.

*/
QString MvdPathResolver::logFile() const
{
	return d->logFile;
}

/*!
	Returns the full path to the application wide temporary directory, which is usually
	in a sub folder of the system's temporary directory.

	<table>
		<tr><th>Platform</th><th>Scope</th><th>Location</th><th>Notes</th></tr>
		<tr><td>Windows</td><td></td><td>{TEMP}\{Org}\{App}.{Time_t}\ or {TMP}\{Org}\{App}.{Time_t}\</td><td></td>
		<tr><td>Unix/Mac</td><td></td><td>/tmp/{Org}/{App}.{Time_t}/</td><td></td>
	</table>

	If a directory with the same name already exists it is removed with all its contents.
*/
QString MvdPathResolver::tempDir() const
{
	return d->tempDir;
}

/*!
	Generates a new unique directory inside of the MvdPathResolver::tempDir() directory.
	The directory name is a unique random string.
	The returned paths has been cleaned by MvdCore::toLocalFilePath() and has a trailing
	path separator character.
*/
QString MvdPathResolver::generateTempDir() const
{
	static const int MaxRetries = 20;

	QString path, uuid;
	int count = 0;
	do
	{
		count++;
		uuid = QUuid::createUuid().toString().replace("{", "").replace("}", "");
		path = tempDir().append(uuid).append("/");
		QDir dir(path);
		if (!dir.exists())
		{
			dir.mkpath(path);
			count = -1;
		}
	}
	while (count >= 0 && count <= MaxRetries);

	if (count == MaxRetries) {
		Q_ASSERT_X(false, "MvdPathResolver::generateTempDir()", "Failed to generate a unique filename.");
	}

	return MvdCore::toLocalFilePath(path, true);
}

/*!
	Returns the absolute path of the directory containing the application executable.
	On Mac OS X, it returns the path of the directory containing the bundle if a bundle
	is detected.
*/
QString MvdPathResolver::applicationDirPath()
{
	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_WS_MAC
	//! \todo The following code needs to be tested on a Mac!
	// We might be in a bundle: Movida.app/Contents/MacOS/Movida
	QDir d(path);
	d.cdUp();
	d.cdUp();
	QFileInfo fi(d);
	if (fi.isBundle())
		path = fi.absolutePath();
#endif
	return MvdCore::toLocalFilePath(path, true);
}

/*!
	Returns the absolute path of a directory that can be used to store
	user specific or system wide resources, such as skins, configuration files
	and more.

	The user resources directory is assured to exist (unless initialization failed), but the
	system resources directory could not exist or it could not have write permissions.

	<table>
		<tr><th>Platform</th><th>Scope</th><th>Location</th><th>Notes</th></tr>
		<tr><td>Windows NT</td><td>System</td><td>{{SSF\Common AppData}}\{Org}\{App}\Resources\</td><td></td>
		<tr><td>Windows NT</td><td>User</td><td>{{USF\AppData}}\{Org}\{App}\Resources\</td><td></td>
		<tr><td>Windows 9x</td><td></td><td>{AppDir}\Resources\</td><td></td>
		<tr><td>Unix</td><td>System</td><td>/etc/{Org}/{App}/Resources/</td><td></td>
		<tr><td>Unix</td><td>User</td><td>{HOME}/.{Org}/{App}/Resources/</td><td></td>
		<tr><td>Mac OS X</td><td>System</td><td>{Bundle}/Contents/Resources/</td><td></td>
		<tr><td>Mac OS X</td><td>User</td><td>{HOME}/Library/{App}/Resources/</td><td></td>
	</table>
*/
QString MvdPathResolver::resourcesDir(Movida::Scope scope) const
{
	return scope == UserScope ? d->resourcesDirUser : d->resourcesDirSystem;
}

/*!
	Removes the directory with given path and all its contents,
	excluding any directory named \p excludeDir.
 */
bool MvdPathResolver::removeDirectoryTree(const QString& path, const QString& excludeDir)
{
	// CleanPath needs to be called or dirName() might fail.
	// This is because of the current (Qt 4.2.x) implementation
	// of QDir.
	QDir dir(QDir::cleanPath(path));
	QString thisDirName = dir.dirName().toLower();
	bool exclude = thisDirName == excludeDir.toLower();

	if (exclude)
		return true;

	QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs
		| QDir::NoSymLinks | QDir::NoDotAndDotDot);
	bool ok = true;

	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fi = list.at(i);

		if (fi.isDir())
		{
			if (!removeDirectoryTree(fi.filePath(), excludeDir))
				ok = false;
		}

		if (!QFile::remove(fi.absoluteFilePath()))
			ok = false;
	}

	if (!exclude)
		dir.rmdir(dir.absolutePath());

	return ok;
}

//! Convenience method to access the MvdPathResolver singleton.
MvdPathResolver& Movida::paths()
{
	return MvdPathResolver::instance();
}
