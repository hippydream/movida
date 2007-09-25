/**************************************************************************
** Filename: pathresolver.cpp
** Revision: 3
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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
**************************************************************************/

#include "pathresolver.h"
#include "global.h"
#include "core.h"
#include <QDir>
#include <QString>
#include <QCoreApplication>
#include <QtGlobal>
#include <QUuid>
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

/*!
	\class MvdPathResolver pathresolver.h
	\ingroup MvdCore Singletons

	\brief Handles platform-specific directory or file paths.
	The returned paths always end with a directory separator.

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
		{Idx} refers to a unique numeric ID
		{Bundle} refers to the path of the application bundle on Mac OS X (i.e. /Applications/Movida.app)
	\endverbatim

	Values between double curly brackets (i.e. "{{SSF\Common AppData}}" refer to Windows registry entries,
	with the following abbreviations being used:
	\verbatim
		SSF = "HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders\"
		USF = "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders\"
	\endverbatim

	<b>Movida::paths()</b> can be used as a convenience method to access the singleton.
	<b>Movida::pid()</b> can be used to retrieve the process ID.
*/


/************************************************************************
MvdPathResolver_P
*************************************************************************/

class MvdPathResolver_P
{
public:
	MvdPathResolver_P();

	bool initialized;

	QString settingsDir;
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
MvdPathResolver_P::MvdPathResolver_P()
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
			qDebug() << "MvdPathResolver: Failed to create directory" << appData;
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
	//! \todo Linux and mac settings!
#endif

	QString sd(QString(this->settingsDir).append(org).append(QDir::separator()).append(app).append(".xml"));
	qDebug() << "MvdPathResolver: storing application settings in" << sd << ".";


	//// 1. temp dir
	QString tempRoot = QDir::tempPath().append("/").append(org).append("/").append(app).append(".");
	QString tempDirPath = tempRoot.append(QString::number(Movida::pid()));
	QFileInfo tempDirInfo(tempDirPath);
	tempDirPath = tempDirInfo.absolutePath();
	if (tempDirInfo.exists())
	{
		bool failed = false;
		if (tempDirInfo.isFile())
			failed = !QFile::remove(tempDirPath);
		else failed = !MvdPathResolver::removeDirectoryTree(tempDirPath);
		if (failed)
		{
			qDebug() << "MvdPathResolver: Failed to create a temporary directory" << tempDirPath;
			return; // initialized is false and the app is supposed to terminate
		}
	}
	
	QDir tempDir(tempDirPath);
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
				qDebug() << "MvdPathResolver: failed to create directory" << root;
				continue;
			}
			root = MvdCore::toLocalFilePath(root, true);
			if (i == 0)
				this->resourcesDirUser = root;
			else
				this->resourcesDirSystem = root;
		}
		resourcesOk = !this->resourcesDirSystem.isEmpty() && !this->resourcesDirUser.isEmpty();
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
			qDebug() << "MvdPathResolver: failed to create directory" << root;
			return;
		}
		root = MvdCore::toLocalFilePath(root, true);
		if (this->resourcesDirSystem.isEmpty())
			this->resourcesDirSystem = root;
		if (this->resourcesDirUser.isEmpty())
			this->resourcesDirUser = root;
	}
#endif
#if defined(Q_WS_MAC)
	QString userResources = QDir::homePath().append("/Library/").append(app).append("/Resources");
	QDir dir(userResources);
	userResources = dir.absolutePath();
	if (!QDir::exists(userResources) && !dir.mkpath(userResources))
		qDebug() << "MvdPathResolver: failed to create directory" << userResources;
	else
		this->resourcesDirUser = MvdCore::toLocalFilePath(userResources, true);

	QString sysResources = QCoreApplication::applicationDirPath().append("/../Resources");
	sysResources = QDir::cleanPath(sysResources);
	QDir sysResourcesDir(sysResources);
	sysResources = sysResourcesDir.absolutePath();
	if (sysResourcesDir.exists())
	{
		qDebug() << "MvdPathResolver: failed to locate directory" << sysResources;
		return;
	}
	sysResources = MvdCore::toLocalFilePath(sysResources, true);
	if (this->resourcesDirUser.isEmpty())
		this->resourcesDirUser = sysResources;
	this->resourcesDirSystem = sysResources;
#endif
#if defined(Q_WS_X11)
	QString userResources = QDir::homeDirPath().append("/.").append(org).append("/").append(app).append("/Resources");
	QDir dir(userResources);
		userResources = dir.absolutePath();
	if (!QDir::exists(userResources) && !dir.mkpath(userResources))
		qDebug() << "MvdPathResolver: failed to create directory" << userResources;
	else
		this->resourcesDirUser = MvdCore::toLocalFilePath(userResources, true);

	QString sysResources = QString("/etc/").append(org).append("/").append(app).append("/Resources");
	dir = QDir(sysResources);
	sysResources = sysResourcesDir.absolutePath();
	if (!QDir::exists(sysResources) && !dir.mkpath(sysResources))
	{
		qDebug() << "MvdPathResolver: failed to create directory" << sysResources;
		return;
	}
	sysResources = MvdCore::toLocalFilePath(sysResources, true);
	if (this->resourcesDirUser.isEmpty())
		this->>resourcesDirUser = sysResources;
	this->resourcesDirSystem = sysResources;
#endif

	qDebug() << "MvdPathResolver: using" << this->resourcesDirUser << "as user resources directory.";
	qDebug() << "MvdPathResolver: using" << this->resourcesDirSystem << "as system resources directory.";


	//// 3. log file

	QString log = this->resourcesDirUser;
	log.append(app).append(".log");
	qDebug() << "MvdPathResolver: writing log to" << log << ".";
	this->logFile = log;

	initialized = true;
}

#ifdef Q_WS_WIN

/*!
	\internal Returns the path to a system shell folder using SHGetSpecialFolderPathW or
	SHGetSpecialFolderPathA from shell32.dll.
*/
QString MvdPathResolver_P::specialFolder(int type)
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
MvdPathResolver* MvdPathResolver::mInstance = 0;

//!	\internal Private constructor - initialize paths.
MvdPathResolver::MvdPathResolver()
{
	d = new MvdPathResolver_P();
}

//! Returns the unique application instance.
MvdPathResolver& MvdPathResolver::instance()
{
	if (mInstance == 0)
		mInstance = new MvdPathResolver();

	return *mInstance;
}

//! Destructor.
MvdPathResolver::~MvdPathResolver()
{
	if (this == mInstance)
	{
		delete d;
	}
	else delete mInstance;
}

//! Returns false if some required directory or file could not be created.
bool MvdPathResolver::isInitialized() const
{
	return d->initialized;
}

/*!
	Returns the absolute path of a directory that contains the user specific settings
	file.

	<table>
		<tr><th>Platform</th><th>Scope</th><th>Location</th><th>Notes</th></tr>
		<tr><td>Windows NT</td><td></td><td>{{USF\AppData}}\{Org}\{App}\</td><td></td>
		<tr><td>Windows 9x</td><td></td><td>{AppDir}\</td><td></td>
		<tr><td>Unix / Mac OS X</td><td></td><td>{HOME}/.{Org}/{App}/</td><td></td>
	</table>
*/
QString MvdPathResolver::settingsDir() const
{
	return d->settingsDir;
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
		<tr><td>Windows</td><td></td><td>{TEMP}\{Org}\{App}.{Pid}\ or {TMP}\{Org}\{App}.{Pid}\</td><td></td>
		<tr><td>Unix/Mac</td><td></td><td>/tmp/{Org}/{App}.{Pid}/</td><td></td>
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
*/
QString MvdPathResolver::generateTempDir() const
{
#define MVD_PATHRESOLVER_MAX_RETRIES 20

	QString path;
	int count = 0;
	do
	{
		count++;
		path = tempDir().append(QUuid::createUuid().toString()).append("/");
		QDir dir(path);
		if (!dir.exists())
		{
			dir.mkpath(path);
			count = -1;
		}
	}
	while (count >= 0 && count <= MVD_PATHRESOLVER_MAX_RETRIES);

	if (count == MVD_PATHRESOLVER_MAX_RETRIES) {
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

//! Returns the application process ID.
int Movida::pid()
{
#ifdef Q_WS_WIN
	return (int) GetCurrentProcessId();
#else
	return (int) getpid();
#endif // Q_WS_WIN
}

//! Convenience method to access the MvdPathResolver singleton.
MvdPathResolver& Movida::paths()
{
	return MvdPathResolver::instance();
}
