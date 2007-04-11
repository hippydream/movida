/**************************************************************************
** Filename: pathresolver.cpp
** Revision: 1
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
#include "logger.h"

#include <QDir>
#include <QString>
#include <QCoreApplication>
#include <QtGlobal>

#ifdef Q_WS_WIN
#include <qt_windows.h>
#else
#include <unistd.h>
#endif

#include <time.h>

using namespace Movida;

/*!
	\class MvdPathResolver pathresolver.h
	\ingroup movidacore

	\brief Handles platform-specific directory or file paths.
	\todo MAC OS X: ~/Library/Application Support/APPLICATIONNAME
*/


/************************************************************************
MvdPathResolver_P
*************************************************************************/

class MvdPathResolver_P
{
public:
	MvdPathResolver_P();

	bool initialized;

	QString logFile;
	QString preferencesFile;
	QString preferencesDir;
	QString companyDir; // Always empty WIN9X platforms
	QString tempBaseDir;
	QString tempDir;

#ifdef Q_WS_WIN
	QString getShellFolder(const QString& folder);
	QString getEnvVariable(const QString& var);
	QString getHomeDir();
	QString getWinTemp();
	QString getSystemRoot();

	bool isNtBased;
#endif

	static int pid;
};

#ifdef Q_WS_WIN
int MvdPathResolver_P::pid = (int) GetCurrentProcessId();
#else
int MvdPathResolver_P::pid = (int) getpid();
#endif // Q_WS_WIN

//! \internal Initializes paths.
MvdPathResolver_P::MvdPathResolver_P()
{
	initialized = false;

	QString root;

#ifdef Q_WS_WIN

	isNtBased = (bool)(QSysInfo::WindowsVersion & QSysInfo::WV_NT_based);

	if (isNtBased)
	{
		root = getShellFolder("AppData");
		if (!root.isEmpty())
		{
			QDir d(root);
			if (!d.exists())
				root.clear();
		}

		if (root.isEmpty())
			root = getHomeDir();
	}

	if (!root.isEmpty())
	{
		QDir d(root);
		if (!d.exists())
			root.clear();
	}

	// Store settings in directory containing our binary (it's the only option with WIN9x)
	if (root.isEmpty())
		root = QString(QCoreApplication::applicationDirPath());

	QChar c = root.at(root.length() - 1);
	if (c != '/' && c != '\\')
		root.append("\\");

#else

	root = QDir::homePath();

	QChar c = root.at(root.length() - 1);
	if (c != '/' && c != '\\')
		root.append("/");

#endif // Q_WS_WIN

#ifdef Q_WS_WIN

	if (isNtBased)
	{
		companyDir = QString(QString("%1%2\\").arg(root).arg(COMPANY_DIR));
		preferencesDir = QString(QString("%1%2\\%3\\").arg(root).arg(COMPANY_DIR).arg(APP_DIR));
	}
	else preferencesDir = QString(QString("%1%2").arg(root).arg(APP_DIR));

#else

	companyDir = QString(QString("%1%2/").arg(root).arg(COMPANY_DIR));
	preferencesDir = QString(QString("%1%2/%3").arg(root).arg(COMPANY_DIR).arg(APP_DIR));

#endif // Q_WS_WIN

	preferencesFile = QString(QString("%1%2").arg(preferencesDir).arg(PREF_FILENAME));
	logFile = QString(QString("%1%2").arg(preferencesDir).arg(LOG_FILENAME));

	root.clear();

#ifdef Q_WS_WIN
	if (isNtBased)
	{
		root = getWinTemp();
		if (!root.isEmpty())
		{
			QDir d(root);
			if (!d.exists())
				root.clear();
		}

		if (root.isEmpty())
			root = getShellFolder("Local Settings");
	}

	if (!root.isEmpty())
	{
		QDir d(root);
		if (!d.exists())
			root.clear();
	}

	if (root.isEmpty())
	{
		root = getSystemRoot();
		c = root.at(root.length() - 1);
		if (c != '/' && c != '\\')
			root.append("\\");
		tempBaseDir = QString(root.append("temp\\"));
	}
	else
	{
		c = root.at(root.length() - 1);
		if (c != '/' && c != '\\')
			root.append("\\");
		tempBaseDir = QString(root);
	}

	tempDir = QString(QString("%1%2\\%3.%4\\").arg(tempBaseDir).arg(COMPANY_DIR).arg(APP_DIR).arg(pid));

#else

	tempBaseDir = "/tmp/";
	tempDir = QString("%1%2/%3.%4/").arg(tempBaseDir).arg(COMPANY_DIR).arg(APP_DIR).arg(pid);

#endif // Q_WS_WIN


	// CREATE MISSING DIRECTORIES

	QDir d(companyDir);
	if (!d.exists())
	{
		if (!d.mkdir(companyDir))
		{
			eLog() << QCoreApplication::translate("MvdPathResolver", "Unable to create directory: %1").arg(companyDir);
			return;
		}
	}

	d = QDir(preferencesDir);
	if (!d.exists())
	{
		if (!d.mkdir(preferencesDir))
		{
			eLog() << QCoreApplication::translate("MvdPathResolver", "Unable to create directory: %1").arg(preferencesDir);
			return;
		}
	}

	QString dname = QString("%1%2").arg(tempBaseDir).arg(COMPANY_DIR);

	d = QDir(dname);
	if (!d.exists(dname))
	{
		if (!d.mkdir(dname))
		{
			eLog() << QCoreApplication::translate("MvdPathResolver", "Unable to create directory: %1").arg(dname);
			return;
		}
	}

	dname = QString("%1%2/%3.%4/").arg(tempBaseDir).arg(COMPANY_DIR).arg(APP_DIR).arg(pid);
	d = QDir(dname);
	if (!d.exists())
	{
		if (!d.mkdir(dname))
		{
			eLog() << QCoreApplication::translate("MvdPathResolver", "Unable to create directory: %1").arg(dname);
			return;
		}
	}

	initialized = true;
}

#ifdef Q_WS_WIN

/*!
	\internal Returns the path to a system shell folder by retrieving it 
	from the registry.
*/
QString MvdPathResolver_P::getShellFolder(const QString& folder)
{
	HKEY hkey = NULL;
	int res;
	char buf[1024];
	DWORD bsz = sizeof(buf);

	QT_WA(
	{
		res = RegOpenKeyExW(
			HKEY_CURRENT_USER,
			reinterpret_cast<const wchar_t *>(QString("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders").utf16()),
			0, KEY_READ, &hkey );
	},
	{
		res = RegOpenKeyExA(
			HKEY_CURRENT_USER,
			QString("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders").toLocal8Bit(),
			0, KEY_READ, &hkey );
	}
	);

	if (res != ERROR_SUCCESS)
	{
		wLog() << QCoreApplication::translate("MvdPathResolver", "Unable to read shell folders from registry");
		return QString();
	}

	QT_WA(
	{
		res = RegQueryValueExW(hkey, reinterpret_cast<const wchar_t *>(folder.utf16()), 
			0, 0, (LPBYTE)buf, &bsz);
		if (res == ERROR_SUCCESS)
			return QString::fromUtf16((unsigned short*) buf);
	},
	{
		res = RegQueryValueExA(hkey, folder.toLocal8Bit(), 0, 0, (LPBYTE)buf, &bsz);
		if (res == ERROR_SUCCESS)
			return QString::fromLatin1(buf);
	}
	);

	return QString();
}

/*!
	\internal Returns the value of an environment variable.
*/
QString MvdPathResolver_P::getEnvVariable(const QString& var)
{
	int res;
	char buf[1024];
	DWORD bsz = sizeof(buf);

	QT_WA(
	{
		res = GetEnvironmentVariable(reinterpret_cast<const wchar_t *>(var.utf16()), 
			(LPWSTR)buf, bsz);
		if (res > 0)
			return QString::fromUtf16((unsigned short*) buf);
		else return QString();
	},
	{
		res = GetEnvironmentVariable((LPWSTR)var.toLocal8Bit().data(), 
			(LPWSTR)buf, bsz);
		if (res > 0)
			return QString::fromLatin1(buf);
		else return QString();
	}
	);

	return QString();
}

/*!
	\internal Returns the path to the users home directory (returns an empty
	string on single-user platforms like Windows 9x).
*/
QString MvdPathResolver_P::getHomeDir()
{
	QString base;
	QString path;

	base = getEnvVariable("HOMEDRIVE");
	if (base.isEmpty())
		return QString();

	path = getEnvVariable("HOMEPATH");
	if (path.isEmpty())
		return QString();

	base.append(path);
	return base;
}

/*!
	\internal Returns the path to the temporary system directory.
*/
QString MvdPathResolver_P::getWinTemp()
{
	QString tmp = getEnvVariable("TEMP");
	return tmp.isEmpty() ? getEnvVariable("TMP") : tmp;
}

/*!
	\internal Returns the path to the system root. This is usually
	c:\\windows on Windows XP and Windows 9x
	and c:\\winnt on Windows NT and Windows 2000
*/
QString MvdPathResolver_P::getSystemRoot()
{
	QString sysRoot = getEnvVariable("SystemRoot");
	if (!sysRoot.isEmpty())
		return sysRoot;

	sysRoot = getEnvVariable("windir");
	if (!sysRoot.isEmpty())
		return sysRoot;

	bool isNt = (QSysInfo::WindowsVersion & QSysInfo::WV_2000) || 
		(QSysInfo::WindowsVersion & QSysInfo::WV_NT);

	return isNt ? "c:\\winnt\\" : "c:\\windows\\";
}

#endif // WIN32


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
	Returns the full path to the application preferences file, which is
	%LOCALSETTINGS_SHELL_FOLDER%\Company\Application.ini on Windows NT based OSes,
	%APPLICATION_PATH%\Application.ini on Windows 9x,
	%USER_HOME%/.company/application/application.rc on Unix based OSes. 
*/
QString MvdPathResolver::preferencesFile() const
{
	return d->preferencesFile;
}

/*!
	Returns the full path to the application log file, which is
	%LOCALSETTINGS_SHELL_FOLDER%\Company\Application.log on Windows NT based OSes,
	%APPLICATION_PATH%\Application.log on Windows 9x,
	%USER_HOME%/.company/application/application.log on Unix based OSes. 
*/
QString MvdPathResolver::logFile() const
{
	return d->logFile;
}

/*!
	Returns the full path to the application preferences directory.
	\sa preferencesFile()
*/
QString MvdPathResolver::preferencesDir() const
{
	return d->preferencesDir;
}

/*!
	Returns the full path to the company preferences directory.
	\sa MvdPathResolver::preferencesFile()
	\warning The company directory is not defined on Windows 9x!
*/
QString MvdPathResolver::companyDir() const
{
	return d->companyDir;
}

/*!
	Returns the full path to the OS temporary directory, which is
	%LOCALSETTINGS_SHELL_FOLDER%\Temp on Windows NT based OSes,
	%WINDOWS_DIR%\Temp on Windows 9x,
	/tmp on Unix based platforms. 
*/
QString MvdPathResolver::tempDir() const
{
	return d->tempDir;
}

/*!
	Generates a new unique directory in the system temporary directory.
	The directory name is partially generated using this pattern:
	APPLICATION_NAME.APPLICATION_PID.RANDOM_NUMBER
*/
QString MvdPathResolver::generateTempDir() const
{
	srand(time(NULL) ^ 3141592654UL);
	bool dirCreated = false;
	
	QString path;

	while (!dirCreated)
	{
		path = QString("%1%2.%3.%4/").arg(d->tempDir).arg(APP_DIR)
			.arg(d->pid).arg(rand() % 10000);
		QDir dir(path);

		if (!dir.exists())
		{
			dir.mkpath(path);
			dirCreated = true;
		}
	}

	return path.append("/");
}

/*!
	Removes the directory with given path and all its contents, 
	excluding any directory	named \p excludeDir.
 */
bool MvdPathResolver::removeDirectoryTree(const QString& path, 
	const QString& excludeDir) const
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
int MvdPathResolver::pid()
{
	return MvdPathResolver_P::pid;
}

//! Convenience method to access the MvdPathResolver singleton.
MvdPathResolver& Movida::paths()
{
	return MvdPathResolver::instance();
}
