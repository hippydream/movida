/**************************************************************************
** Filename: core.cpp
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

#include "core.h"
#include "settings.h"
#include "pathresolver.h"
#include "logger.h"
#include "shareddata.h"

#include <QApplication>
#include <QLocale>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QDate>
#include <QStringList>
#include <QVarLengthArray>
#include <QDebug>

using namespace Movida;

/*!
	\class MvdCore core.h
	\ingroup MvdCore

	\brief Application core initialization and utility methods.
*/


/************************************************************************
MvdCore_P
*************************************************************************/

//! \internal
class MvdCore_P
{
public:
	MvdCore_P()
	{
		//! \todo Constants might be better added to a text file at build time
		parameters.insert("movidacore-max-rating", 5);
		parameters.insert("movidacore-max-running-time", 999);
		parameters.insert("movidacore-imdb-id-length", 7);
		parameters.insert("movidacore-imdb-id-regexp", "[0-9]{7}");

		// The Oberammergau Passion Play of 1898 was the first commercial 
		// motion picture ever produced. :)
		// It's quite hard that someone will add it to Movida, but its a 
		// nice date ;)
		parameters.insert("movidacore-min-movie-year", 1898);

		// Movie posters bigger than this will be scaled.
		parameters.insert("movidacore-max-poster-kb", 512);
		parameters.insert("movidacore-max-poster-size", QSize(400, 150));
	}

	QDate globalSmdDate(const QString& s) const;
	void loadGlobalSmdData(const QString& baseDir, Movida::SmdDataRole role) const;

	QHash<QString,QVariant> parameters;
};

//! \internal
MvdCore_P* MvdCore::d = 0;

//! \internal
QDate MvdCore_P::globalSmdDate(const QString& s) const
{
	iLog() << QString("MvdCore: Checking date in %1.").arg(s);

	QDate date;
	QFile file(s);

	if (file.open(QIODevice::ReadOnly))
	{
		QTextStream s(&file);
		s.setCodec( "utf-8" );

		QString line = s.readLine();
		QRegExp rx("^movida_global_smd\\:([0-9]{4}-[0-9]{2}-[0-9]{2})$");

		if (rx.indexIn(line) != -1)
			date = QDate::fromString(rx.cap(1), Qt::ISODate);

		file.close();
	}

	return date;
}

//! \internal
void MvdCore_P::loadGlobalSmdData(const QString& baseDir, Movida::SmdDataRole role) const
{
	QString locale = QLocale::system().name();
	QString defLocale = QLocale(QLocale::English, QLocale::UnitedKingdom).name();

	QString file;
	switch (role)
	{
	case Movida::PersonRole: file = "persons.xml"; break;
	case Movida::GenreRole: file = "genres.xml"; break;
	case Movida::TagRole: file = "tags.xml"; break;
	case Movida::UrlRole: file = "urls.xml"; break;
	case Movida::CountryRole: file = "countries.xml"; break;
	case Movida::LanguageRole: file = "languages.xml"; break;
	default: ;
	}

	QDate dLocal = globalSmdDate(QString("%1/smd_%2/%3").arg(baseDir).arg(locale).arg(file));
	QDate dEnglish = globalSmdDate(QString("%1/smd_%2/%3").arg(baseDir).arg(defLocale).arg(file));
	QDate dEmbedded = globalSmdDate(QString(":/smd/%1").arg(file));

	QString selected;
	QList<QDate> l;
	l << dLocal << dEnglish << dEmbedded;
	qSort(l);

	QDate selDate;
	selDate = l.takeLast();
	if (!selDate.isValid())
		selDate = l.takeLast();
	if (!selDate.isValid())
		selDate = l.takeLast();
	
	if (!selDate.isValid())
	{
		return;
	}

	if (selDate == dLocal)
		selected = QString("%1/smd_%2/%3").arg(baseDir).arg(locale).arg(file);
	else if (selDate == dEnglish)
		selected = QString("%1/smd_%2/%3").arg(baseDir).arg(defLocale).arg(file);
	else selected = QString(":/smd/%3").arg(file);

	iLog() << QString("MvdCore: Loading %1.").arg(selected);

	QFile f(selected);
	if (!f.open(QIODevice::ReadOnly))
	{
		eLog() << QString("MvdCore: Failed to load %1.").arg(selected);
		return;
	}
	
	QTextStream s(&f);
	s.setCodec("utf-8");

	QString line = s.readLine();
	QRegExp rx("^movida_global_smd\\:([0-9]{4}-[0-9]{2}-[0-9]{2})$");

	if (rx.indexIn(line) != -1)
		selDate = QDate::fromString(rx.cap(1), Qt::ISODate);
	else
	{
		eLog() << QString("MvdCore: Bad file format: %1.").arg(selected);
		return;
	}

	MvdSharedData& smd = Movida::globalSD();

	while (!(line = s.readLine()).isNull())
	{
		line = line.trimmed();

		if (line.isEmpty())
			continue;

		if (role == Movida::PersonRole)
		{
			QStringList list = line.split("\t", QString::KeepEmptyParts);
			if (list.size() == 1)
				smd.addPerson(QString(), list.at(0));
			else if (list.size() == 2)
				smd.addPerson(list.at(1), list.at(0));
			else if (list.size() > 2)
			{
				QList<smdid> urls;
				for (int i = 2; i < list.size(); ++i)
				{
					bool ok;
					int n = list.at(i).toInt(&ok);
					if (ok)
						urls.append(n);
				}

				smd.addPerson(list.at(1), list.at(0), urls);
			}
		}
		else if (role == Movida::UrlRole)
		{
			QStringList list = line.split("\t", QString::KeepEmptyParts);
			if (list.size() == 2)
				smd.addUrl(list.at(0), list.at(1));
		}
		else
		{
			switch (role)
			{
			case Movida::TagRole: smd.addTag(line); break;
			case Movida::GenreRole: smd.addGenre(line); break;
			case Movida::CountryRole: smd.addCountry(line); break;
			case Movida::LanguageRole: smd.addLanguage(line); break;
			default: ;
			}
		}
	}

	f.close();
}


/************************************************************************
 MvdCore
*************************************************************************/

//! \internal
bool MvdCore::MvdCoreInitialized = false;

/*!
	Call this possibly from the main routine. It will attempt to initialize
	any required directories, files and libraries.
	Exit if the method returns an error. Running Movida if some error occurs
	may lead to an unpredictable behavior.
	Subsequent calls to this method have no effect. they simply return the 
	return value of the first initialization.
*/
bool MvdCore::initCore()
{
	if (d != 0)
		return MvdCoreInitialized;

	// Pre-initialize MvdPathResolver to create missing application directories
	if (!paths().isInitialized())
		return false;

	d = new MvdCore_P;

	/*
		Load global SD data.
		User-files are in PREFERENCES_DIR/smd_XX where XX is a two char country code.
		Search order: localized file, English file or embedded file.
		If the embedded or English files are newer they will be loaded instead of the
		localized file.
	*/

	QString prefDir = paths().preferencesDir();

	d->loadGlobalSmdData(prefDir, Movida::PersonRole);
	d->loadGlobalSmdData(prefDir, Movida::UrlRole);
	d->loadGlobalSmdData(prefDir, Movida::TagRole);
	d->loadGlobalSmdData(prefDir, Movida::GenreRole);
	d->loadGlobalSmdData(prefDir, Movida::CountryRole);
	d->loadGlobalSmdData(prefDir, Movida::LanguageRole);

	//! \todo check library versions?

	MvdCoreInitialized = true;
	return true;
}

/*!
	Call this once on application startup to load application preferences, 
	global SD and more.
	Subsequent calls to this method have no effect.
	Set non-core default settings in the XmlPreferences singleton before 
	calling this method so that they can be possibly overwritten by user 
	defined settings.
*/
void MvdCore::loadStatus()
{
	// Load application preferences. Set defaults and overwrite with user settings.
	MvdSettings& s = settings();

	// check or write out checksum when loading/saving Movida archives
	s.setBool("check_hash", false, "collection");
	s.setBool("write_hash", true, "collection");

	QString prefFile = paths().preferencesFile();
	iLog() << QString("MvdCore: Loading preferences from %1").arg(prefFile);

	if (!s.load(prefFile))
		wLog() << QString("MvdCore: Unable to load preferences. Using default settings.");
}

/*!
	Call this once on application termination to store application preferences 
	and more.
	Subsequent calls to this method have no effect.
	Write non-core settings in the XmlPreferences singleton before calling 
	this method as it will write the settings to the preferences file!
*/
void MvdCore::storeStatus()
{
	Q_ASSERT(d);

	QString prefFile = paths().preferencesFile();
	iLog() << QString("MvdCore: Storing preferences to %1").arg(prefFile);

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

	qDebug() <<	"Request for unbound parameter" << name;
	return QVariant();
}

/*!
	Registers application parameters.
	Please use a library unique prefix for the names, like "gui-myparam".
*/
void MvdCore::registerParameters(const QHash<QString,QVariant>& p)
{
	Q_ASSERT(d);

	d->parameters.unite(p);
}

/*!
	atoi() for unsigned 32bit values.
*/
quint32 MvdCore::atoui32(const char* c)
{
	QString s(c);
	bool ok;
	quint32 i;
	i = s.toUInt(&ok);
	return ok ? i : 0;
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
