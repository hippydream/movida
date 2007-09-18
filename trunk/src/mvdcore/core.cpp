/**************************************************************************
** Filename: core.cpp
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

#include "core.h"
#include "settings.h"
#include "pathresolver.h"
#include "logger.h"
#include "shareddata.h"
#include <QCoreApplication>
#include <QLocale>
#include <QSize>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegExp>
#include <QDate>
#include <QStringList>
#include <QVarLengthArray>
#include <QProcess>
#include <QtGlobal>
#include <QDir>
#include <QDebug>

using namespace Movida;

/*!
	\class MvdCore core.h
	\ingroup MvdCore

	\brief Application core: initialization and utility methods.
*/


/************************************************************************
MvdCore_P
*************************************************************************/

//! \internal
class MvdCore_P
{
public:
	//! \internal
	MvdCore_P()
	{
		parameters.insert("mvdp://mvdcore/max-rating", 5);
		parameters.insert("mvdp://mvdcore/max-running-time", 999);
		parameters.insert("mvdp://mvdcore/imdb-id-length", 7);
		parameters.insert("mvdp://mvdcore/imdb-id-regexp", "[0-9]{7}");
		parameters.insert("mvdp://mvdcore/imdb-id-mask", "9999999;0");

		// The Oberammergau Passion Play of 1898 was the first commercial
		// motion picture ever produced. :)
		// It's quite hard that someone will add it to Movida, but its a
		// nice date ;)
		parameters.insert("mvdp://mvdcore/min-movie-year", 1898);

		// Movie posters bigger than this will be scaled.
		parameters.insert("mvdp://mvdcore/max-poster-kb", 512);
		parameters.insert("mvdp://mvdcore/max-poster-size", QSize(400, 150));
	}

	QHash<QString,QVariant> parameters;
};

//! \internal
MvdCore_P* MvdCore::d = 0;


/************************************************************************
 MvdCore
*************************************************************************/

//! \internal
bool MvdCore::MvdCoreInitOk = false;

/*!
	Possibly call this from the main() routine. It will attempt to initialize
	any required directories, files and libraries.
	Exit if the method returns an error. Running Movida if some error occurs
	may lead to an unpredictable behavior.
	Subsequent calls to this method have no effect, they simply return the
	exit status of the first initialization.
*/
bool MvdCore::initCore()
{
	if (d != 0)
		return MvdCoreInitOk;

	// Pre-initialize MvdPathResolver to create missing application directories
	if (!paths().isInitialized())
		return false;

	d = new MvdCore_P;

	QString prefDir = paths().preferencesDir();

	//! \todo check library versions?

	// Init libxml2
	xmlSetStructuredErrorFunc(NULL, Movida::xmlStructuredErrorHandler);

	MvdCoreInitOk = true;
	return true;
}

/*!
	Call this once on application startup to load application preferences,
	and more.
	Subsequent calls to this method have no effect.
	Set non-core default settings in the MvdSettings singleton before
	calling this method so that they can be possibly overwritten by user
	defined settings.
*/
void MvdCore::loadStatus()
{
	// Load application preferences. Set defaults and overwrite with user settings.
	MvdSettings& s = settings();

	QString prefFile = paths().preferencesFile();
	iLog() << QString("MvdCore: Loading preferences from %1").arg(prefFile);

	if (!s.load(prefFile))
		wLog() << QString("MvdCore: No preferences loaded. Using default settings.");
}

/*!
	Call this once on application termination to store application preferences
	and more.
	Subsequent calls to this method have no effect.
	Write non-core settings in the MvdSettings singleton before calling
	this method as it will write the settings to the preferences file!
*/
void MvdCore::storeStatus()
{
	Q_ASSERT(d);

	QString prefFile = paths().preferencesFile();
	iLog() << QString("MvdCore: Storing preferences in %1").arg(prefFile);

	if (!settings().save(prefFile))
		eLog() << QString("MvdCore: Unable to store preferences.");

	delete d;
	d = 0;
}

/*!
	Returns the value of a specific application parameter.
	Returns a null QVariant and prints an error message using
	qDebug (this is for developers only, so it makes no sense to
	use the log here - no unbound variables should be used in public
	releases) if the parameter name is not bound.

	Values should be used as constants and client libraries
	should register their parameters at startup.
	This works slower than using #defines or static constants,
	but it speeds up compilation, adds binary compatibility and
	a centralized control point for application wide constants.
*/
QVariant MvdCore::parameter(const QString& name)
{
	Q_ASSERT(d);

	QHash<QString,QVariant>::ConstIterator it = d->parameters.find(name);
	if (it != d->parameters.constEnd())
		return it.value();

	qDebug() << "Request for unbound parameter" << name;
	return QVariant();
}

/*!
	Registers application parameters.
	Please use a library unique prefix for the names.
	movida uses the following format, so consider using the same pattern for consistency:
	"mvdp://LIBRARY/SOME-PARAMETER-NAME" (i.e. "mvdp://mvdcore/imdb-id-regexp")
*/
void MvdCore::registerParameters(const QHash<QString,QVariant>& p)
{
	Q_ASSERT(d);

	d->parameters.unite(p);
}

/*!
	Similar to atoi() for unsigned 32bit values.
*/
mvdid MvdCore::atoid(const char* c, bool* ok)
{
	QString s(c);
	bool myOk;
	mvdid i;
	i = s.toUInt(&myOk);
	if (ok) *ok = myOk;
	return myOk ? i : MvdNull;
}

/*!
	Replaces occurrences of \n with QChar::LineSeparator.
*/
QString MvdCore::replaceNewLine(QString text)
{
	const QChar nl = QLatin1Char('\n');
	for (int i = 0; i < text.count(); ++i)
		if (text.at(i) == nl)
			text[i] = QChar::LineSeparator;
	return text;
}

//! \internal Some code from Qt 4.2.0 qurl.cpp required by toLatin1PercentEncoding()
namespace
{
	inline bool q_strchr(const char str[], char chr)
	{
		if (!str) return false;

		const char *ptr = str;
		char c;
		while ((c = *ptr++))
			if (c == chr)
				return true;
		return false;
	}

	static const char hexnumbers[] = "0123456789ABCDEF";
	static inline char toHex(char c)
	{
		return hexnumbers[c & 0xf];
	}
}

/*!
	Same as QUrl::toPercentEncoding except that it does not convert the
	string to UTF8 but to Latin1 (as required by some servers).

	From the QUrl documentation:

	Returns an encoded copy of input. input is first converted to UTF-8,
	and all ASCII-characters that are not in the unreserved group are percent
	encoded. To prevent characters from being percent encoded pass them to
	exclude. To force characters to be percent encoded pass them to include.

	Unreserved is defined as: ALPHA / DIGIT / "-" / "." / "_" / "~"
*/
QByteArray MvdCore::toLatin1PercentEncoding(const QString& input,
	const QByteArray& exclude, const QByteArray& include)
{
	// The code is the same as QUrl::toPercentEncoding as to Qt 4.2.0.
	// The following line is actually the only difference (except for
	// the removed debug prints) with the original implementation.
	QByteArray tmp = input.toLatin1();

	QVarLengthArray<char> output(tmp.size() * 3);

	int len = tmp.count();
	char *data = output.data();
	const char *inputData = tmp.constData();
	int length = 0;

	const char * dontEncode = 0;
	if (!exclude.isEmpty()) dontEncode = exclude.constData();


	if (include.isEmpty()) {
		for (int i = 0; i < len; ++i) {
			unsigned char c = *inputData++;
			if (c >= 0x61 && c <= 0x7A // ALPHA
				|| c >= 0x41 && c <= 0x5A // ALPHA
				|| c >= 0x30 && c <= 0x39 // DIGIT
				|| c == 0x2D // -
				|| c == 0x2E // .
				|| c == 0x5F // _
				|| c == 0x7E // ~
				|| q_strchr(dontEncode, c)) {
					data[length++] = c;
			} else {
				data[length++] = '%';
				data[length++] = toHex((c & 0xf0) >> 4);
				data[length++] = toHex(c & 0xf);
			}
		}
	} else {
		const char * alsoEncode = include.constData();
		for (int i = 0; i < len; ++i) {
			unsigned char c = *inputData++;
			if ((c >= 0x61 && c <= 0x7A // ALPHA
				|| c >= 0x41 && c <= 0x5A // ALPHA
				|| c >= 0x30 && c <= 0x39 // DIGIT
				|| c == 0x2D // -
				|| c == 0x2E // .
				|| c == 0x5F // _
				|| c == 0x7E // ~
				|| q_strchr(dontEncode, c))
				&& !q_strchr(alsoEncode, c)) {
					data[length++] = c;
			} else {
				data[length++] = '%';
				data[length++] = toHex((c & 0xf0) >> 4);
				data[length++] = toHex(c & 0xf);
			}
		}
	}

	return QByteArray(output.data(), length);
}

/*!
	Replaces entities in XML/HTML text.
*/
QString MvdCore::decodeXmlEntities(QString s)
{
	if (s.isEmpty())
		return s;

	QRegExp rx("&#(\\d+);");
	int pos = rx.indexIn(s);
	while(pos >= 0)
	{
		s.replace(pos, rx.matchedLength(), QChar((rx.cap(1).toInt())));
		pos = rx.indexIn(s, pos + 1);
	}

	s.replace("&nbsp;", QChar(' '));
	return s;
}

//! Redirects libxml2 structured errors to the application log.
void Movida::xmlStructuredErrorHandler(void* userData, xmlErrorPtr error)
{
	Q_UNUSED(userData);

	QString msg = QString("(libxml2@%1:%2) ").arg(error->line).arg(error->int2);
	msg.append(QString(error->message).trimmed());

	switch (error->level)
	{
	case XML_ERR_NONE:
		iLog() << msg; break;
	case XML_ERR_WARNING:
		wLog() << msg; break;
	default:
		eLog() << msg;
	}
}

MvdCore::LabelAction MvdCore::parseLabelAction(const QString& url)
{
	LabelAction a;

	QRegExp rx("^movida://([^/]*)/?([^/]*)?$");
	if (rx.indexIn(url) >= 0)
	{
		a.action = rx.cap(1);
		a.parameter = rx.cap(2);
	}

	return a;
}

/*!
	Attempts to locate an application with the given name.
	\p name must not include platform dependent suffixes (i.e. ".exe").

	The application is first searched in the movida application directory 
	(unless \p searchInAppDirPath is true) and then
	in the directories included in the PATH environment variable.

	Returns the path to the application (filename included) or an empty QString on failure.
*/
QString MvdCore::locateApplication(QString name, bool searchInAppDirPath)
{
	name = name.trimmed();
	if (name.isEmpty())
		return QString();

	Qt::CaseSensitivity cs = Qt::CaseSensitive;

#ifdef Q_OS_WIN
		name.append(".exe");
		cs = Qt::CaseInsensitive;
#endif

	//! \todo On Mac OS X this will point to the directory actually containing the executable, which may be inside of an application bundle (if the application is bundled). 
	QString appPath = searchInAppDirPath ? QCoreApplication::applicationDirPath() : QString();
	if (!appPath.isEmpty())
	{
		QFileInfo fi(appPath.append("/").append(name));
		if (fi.exists() && fi.isExecutable())
			return fi.absoluteFilePath();
	}

	QString pathEnv = env("path", Qt::CaseInsensitive);

#ifdef Q_OS_WIN
	QChar pathSep(';');
#else
	QChar pathSep(':');
#endif

	QStringList pathList = pathEnv.split(pathSep, QString::SkipEmptyParts);
	for (int i = 0; i < pathList.size(); ++i)
	{
		QString p = pathList.at(i);
		QFileInfo fi(p.append("/").append(name));
		if (fi.exists() && fi.isExecutable())
			return fi.absoluteFilePath();
	}

	return QString();
}

//! Returns the value of an environment variable.
QString MvdCore::env(const QString& s, Qt::CaseSensitivity cs)
{
	QRegExp rx(QString("^%1=(.*)$").arg(s));
	rx.setCaseSensitivity(cs);

	QStringList envList = QProcess::systemEnvironment();
	for (int i = 0; i < envList.size(); ++i)
	{
		QString e = envList.at(i);
		if (rx.exactMatch(e))
			return rx.cap(1);
	}

	return QString();
}

//! Cleans a file path and replaces path separators with the separators used on the current platform.
QString MvdCore::toLocalFilePath(QString s)
{
#ifdef Q_WS_WIN
	QChar goodSep('\\');
	QChar badSep('/');
#else
	QChar goodSep('/');
	QChar badSep('\\');
#endif

	return QDir::cleanPath(s).replace(badSep, goodSep);
}
