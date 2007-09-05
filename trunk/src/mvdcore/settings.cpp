/**************************************************************************
** Filename: settings.cpp
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

#include "settings.h"
#include "base64.h"
#include "global.h"
#include "xmlwriter.h"
#include <QByteArray>
#include <QBitArray>
#include <QVariant>
#include <QHash>
#include <QStringList>
#include <QRect>
#ifndef MVD_QTCORE_ONLY
#include <QColor>
#endif
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/*!
	\class MvdSettings settings.h
	\ingroup MvdCore Singletons

	\brief The MvdSettings class allows to easily store and retrieve
	application preferences from XML files.

	The default section is just a convenience section. It beheaves exactly 
	like any other section but you can set its name once and never need to 
	call it again (the default name for this section is "default").

	Adding support for new data types involves the following steps:

	<ul>
		<li>add the get and set methods to the header file interface
		<li>implement a getTYPE method; this usually involves
			<ul>
				<li>changing the type returned in case of error
				<li>changing QVariant::TYPE to your type
				<li>changing return v.data.toTYPE() to your type
			</ul>
		<li>implement a setTYPE method (usually tou only need to change the method 
		parameters)
			<ul>
				<li>add a case entry to the writeSection() method to serialize the data 
				type to a string
				<li>add an else if entry to the parseSection() method to deserialize the 
				data type from a string
			</ul>
	</ul>
*/

/*!
	\enum MvdSettings::BinaryEncoding Identifies how binary data should 
	be encoded.

	\value CsvEncoding Separates values using a simple comma
	\value Base64Encoding Encodes binary data using base 64 encoding
*/


/************************************************************************
MvdSettings_P
*************************************************************************/

//! \internal
class MvdSettings_P
{
public:
	MvdSettings_P();

	typedef struct EncodedVariant
	{
		QVariant data;
		MvdSettings::BinaryEncoding encoding;
	};

	typedef QHash<QString, EncodedVariant> Section;
	typedef QHash<QString, Section> SectionTable;

	SectionTable data;

	QString defaultSectionName;

	QString product;
	QString company;
	quint8 version;

	bool ignoreMissingProductVersion;

	MvdSettings::ErrorCode lastErrorCode;
	MvdSettings::BinaryEncoding encoding;

	void writeSection(MvdXmlWriter* out, const QString& name, const Section& section);
	void parseSection(xmlDocPtr doc, xmlNodePtr rootNode, Section* section);
	bool checkApplicationVersion(quint8 version);

	QVariant getData(const QString& name, const QString& section);
	void setData(const QString& name, const QString& section, const EncodedVariant& data);
};

//! \internal
MvdSettings_P::MvdSettings_P()
{
	defaultSectionName = "default";
	version = 0;
	ignoreMissingProductVersion = true;
	lastErrorCode = MvdSettings::NoError;
	encoding = MvdSettings::Base64Encoding;
}

//! \internal
void MvdSettings_P::writeSection(MvdXmlWriter* out, const QString& name, 
	const Section& section)
{
	QHash<QString,QString> attrs;

	attrs.insert("name", name);
	out->writeOpenTag("section", attrs);
	attrs.clear();

	for (Section::ConstIterator sectionIterator = section.constBegin();
		sectionIterator != section.constEnd(); ++sectionIterator)
	{
		const EncodedVariant& v = sectionIterator.value();

		attrs.insert("name", sectionIterator.key());

		switch (v.data.type())
		{
		case QVariant::String :
			attrs.insert("type", "string");
			out->writeTaggedString("setting", v.data.toString(), attrs);
			break;
		case QVariant::StringList :
			{
				attrs.insert("type", "stringlist");
				out->writeOpenTag("setting", attrs);
				attrs.clear();
				QStringList list = v.data.toStringList();
				for (int i = 0; i < list.size(); ++i)
					out->writeTaggedString("value", list.at(i), attrs);
				out->writeCloseTag("setting");
			}
			break;
		case QVariant::Bool :
			attrs.insert("type", "bool");
			out->writeTaggedString("setting", v.data.toBool() ? "true" : "false", attrs);
			break;
		case QVariant::Int :
			attrs.insert("type", "int");
			out->writeTaggedString("setting", QString::number(v.data.toInt()), attrs);
			break;
		case QVariant::LongLong :
			attrs.insert("type", "int64");
			out->writeTaggedString("setting", QString::number(v.data.toLongLong()), attrs);
			break;
		case QVariant::Rect :
			{
				attrs.insert("type", "rect");
				QRect rect = v.data.toRect();
				QString s = QString("%1;%2;%3;%4").arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
				out->writeTaggedString("setting", s, attrs);
			}
			break;
		case QVariant::Point :
			{
				attrs.insert("type", "point");
				QPoint point = v.data.toPoint();
				QString s = QString("%1;%2").arg(point.x()).arg(point.y());
				out->writeTaggedString("setting", s, attrs);
			}
			break;
		case QVariant::Size :
			{
				attrs.insert("type", "size");
				QSize size = v.data.toSize();
				QString s = QString("%1;%2").arg(size.width()).arg(size.height());
				out->writeTaggedString("setting", s, attrs);
			}
			break;
		case QVariant::ByteArray :
			{
				attrs.insert("type", "bytearray");
				const QByteArray ba = v.data.toByteArray();
				switch (v.encoding)
				{
				case MvdSettings::Base64Encoding:
					attrs.insert("encoding", "base64");
					out->writeTaggedString("setting", MvdBase64::encode(ba), attrs);
					break;
				default:
					attrs.insert("encoding", "csv");
					QString s;
					for (uint i = 0; i < (uint) ba.size(); ++i)
						(i != 0) ? s += "," + QString::number((uint)ba.at(i), 16) : s += QString::number((uint)ba.at(i), 16);
					out->writeTaggedString("setting", s, attrs);
				}
				attrs.clear();
			}
			break;
		case QVariant::BitArray :
			{
				attrs.insert("type", "bitarray");
				const QBitArray ba = v.data.toBitArray();
				attrs.insert("size", QString::number(ba.size()));
				switch (v.encoding)
				{
				case MvdSettings::Base64Encoding:
					attrs.insert("encoding", "base64");
					out->writeTaggedString("setting", MvdBase64::encode(ba), attrs);
					break;
				default:
					attrs.insert("encoding", "csv");
					QString s;
					for (uint i = 0; i < (uint) ba.size(); ++i)
						(i != 0) ? s += ba.testBit(i) ? ",1" : ",0" : s += ba.testBit(i) ? "1" : "0";
					out->writeTaggedString("setting", s, attrs);
				}
				attrs.clear();
			}
			break;
#ifndef MVD_QTCORE_ONLY
		case QVariant::Color :
			attrs.insert("type", "color");
			out->writeTaggedString("setting", v.data.value<QColor>().name(), attrs);
			break;
#endif // MVD_QTCORE_ONLY
		default:
			;
		}
	}

	out->writeCloseTag("section");
}

//! \internal
bool MvdSettings_P::checkApplicationVersion(quint8 v)
{
	if (v == 0)
		return ignoreMissingProductVersion;

	return version == v;
}

//! \internal
void MvdSettings_P::parseSection(xmlDocPtr doc, xmlNodePtr rootNode, 
	Section* section)
{
	xmlNodePtr cur = rootNode->xmlChildrenNode;
	xmlChar* attr = 0;

	while (cur)
	{
		if (xmlStrcmp(cur->name, (const xmlChar*) "setting"))
		{
			cur = cur->next;
			continue;
		}

		attr = xmlGetProp(cur, (const xmlChar*) "name");
		if (!attr)
		{
			cur = cur->next;
			continue;
		}

		QString name((const char*) attr);
		xmlFree(attr);

		attr = xmlGetProp(cur, (const xmlChar*) "type");
		if (!attr)
		{
			cur = cur->next;
			continue;
		}

		QString type((const char*) attr);
		xmlFree(attr);

		if (type == "string")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				EncodedVariant v;
				v.data = QVariant( QString( (const char*)txt ) );
				section->insert(name, v);
				xmlFree(txt);
			}
		}
		else if (type == "stringlist")
		{
			xmlNodePtr listNode = cur->xmlChildrenNode;

			QString listText;
			QStringList list;

			while (listNode)
			{
				xmlChar* txt = xmlNodeListGetString(doc, listNode->xmlChildrenNode, 1);
				if (txt)
				{
					list.append( QString((const char*) txt) );
					xmlFree(txt);
				}

				listNode = listNode->next;
			}

			EncodedVariant v;
			v.data = QVariant(list);
			section->insert(name, v);
		}
		else if (type == "bool")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				EncodedVariant v;
				v.data = QVariant(!xmlStrcmp(txt, (const xmlChar*) "true"));
				section->insert(name, v);
				xmlFree(txt);
			}
		}
		else if (type == "int")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				QString n( (const char*) txt );
				xmlFree(txt);

				bool ok;
				int i = n.toInt(&ok);

				if (ok)
				{
					EncodedVariant v;
					v.data = QVariant(i);
					section->insert(name, v);
				}
			}
		}
		else if (type == "int64")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				QString n( (const char*) txt );
				xmlFree(txt);

				bool ok;
				qlonglong i = n.toLongLong(&ok);

				if (ok)
				{
					EncodedVariant v;
					v.data = QVariant(i);
					section->insert(name, v);
				}
			}
		}
		else if (type == "rect")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				QString n( (const char*) txt );
				xmlFree(txt);

				QStringList lst = n.split(';', QString::KeepEmptyParts);

				// a qrect has 4 values: xpos, ypos, width and height
				if (lst.size() == 4)
				{
					int x, y, w, h;
					int count = 0;
					bool ok;

					x = lst[0].toInt(&ok);
					if (ok) ++count;
					y = lst[1].toInt(&ok);
					if (ok) ++count;
					w = lst[2].toInt(&ok);
					if (ok) ++count;
					h = lst[3].toInt(&ok);
					if (ok) ++count;

					if (count == 4)
					{
						EncodedVariant v;
						v.data = QVariant(QRect(x,y,w,h));
						section->insert(name, v);
					}
				}
			}
		}
		else if (type == "point")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				QString n( (const char*) txt );
				xmlFree(txt);

				QStringList lst = n.split(';', QString::KeepEmptyParts);

				// a qrect has 2 values, the x and y coordinates
				if (lst.size() == 2)
				{
					int x, y;
					int count = 0;
					bool ok;

					x = lst[0].toInt(&ok);
					if (ok) ++count;
					y = lst[1].toInt(&ok);
					if (ok) ++count;

					if (count == 2)
					{
						EncodedVariant v;
						v.data = QVariant(QPoint(x,y));
						section->insert(name, v);
					}
				}
			}
		}
		else if (type == "size")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				QString n( (const char*) txt );
				xmlFree(txt);

				QStringList lst = n.split(';', QString::KeepEmptyParts);

				// a qrect has 2 values: width and height
				if (lst.size() == 2)
				{
					int w, h;
					int count = 0;
					bool ok;

					w = lst[0].toInt(&ok);
					if (ok) ++count;
					h = lst[1].toInt(&ok);
					if (ok) ++count;

					if (count == 2)
					{
						EncodedVariant v;
						v.data = QVariant(QSize(w,h));
						section->insert(name, v);
					}
				}
			}
		}
		else if (type == "bytearray")
		{
			xmlChar* attr = xmlGetProp(cur, (const xmlChar*)"encoding");
			if (attr)
			{
				if (!xmlStrcmp(attr, (const xmlChar*) "base64"))
				{
					xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					if (txt != 0)
					{
						QString n( (const char*) txt );
						xmlFree(txt);

						QByteArray ba = MvdBase64::decode(n);
						EncodedVariant v;
						v.data = QVariant(ba);
						v.encoding = MvdSettings::Base64Encoding;
						section->insert(name, v);
					}
				}
				else if (!xmlStrcmp(attr, (const xmlChar*) "csv"))
				{
					xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					if (txt != 0)
					{
						QString n( (const char*) txt );
						xmlFree(txt);

						QStringList lst = n.split(',', QString::SkipEmptyParts);
						uint sz = lst.size();
						QByteArray ba(sz, '\0');
						bool ok;
						uint count = 0;
						for (QStringList::ConstIterator itr=lst.constBegin(); itr!=lst.constEnd(); ++itr)
						{
							uint n = (*itr).toUInt(&ok,16);
							if (ok)
								ba.data()[count++] = (uchar)n;
						}
						if (count < sz)
							ba.resize(count);
						EncodedVariant v;
						v.data = QVariant(ba);
						v.encoding = MvdSettings::CsvEncoding;
						section->insert(name, v);
					}
				}

				xmlFree(attr);
			}
		}
		else if (type == "bitarray")
		{
			QString n;

			xmlChar* attr = xmlGetProp(cur, (const xmlChar*) "size");
			if (attr)
			{
				QString n((const char*)attr);
				xmlFree(attr);

				bool ok;
				uint arr_sz = n.toUInt(&ok);

				if (!ok)
				{
					cur = cur->next;
					continue;
				}

				attr = xmlGetProp(cur, (const xmlChar*) "encoding");
				if (!attr)
				{
					cur = cur->next;
					continue;
				}

				if (!xmlStrcmp(attr, (const xmlChar*) "base64"))
				{
					xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					if (txt)
					{
						QString n( (const char*) txt );
						xmlFree(txt);

						QBitArray ba = MvdBase64::decode(n, arr_sz);
						EncodedVariant v;
						v.data = QVariant(ba);
						v.encoding = MvdSettings::Base64Encoding;
						section->insert(name, v);
					}
				}
				else if (!xmlStrcmp(attr, (const xmlChar*) "csv"))
				{
					xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
					if (txt)
					{
						QString n( (const char*) txt );
						xmlFree(txt);

						QStringList lst = n.split(',', QString::SkipEmptyParts);
						uint sz = lst.size();
						if (arr_sz < sz)
							sz = arr_sz;
						QBitArray ba(sz);
						uint count = 0;
						for (int i = 0; i < lst.size(); ++i)
						{
							if (lst.at(i) == "1")
								ba.setBit(count++, true);
							else
								ba.setBit(count++, false);
						}
						if (count < sz)
							ba.resize(count);
						EncodedVariant v;
						v.data = QVariant(ba);
						v.encoding = MvdSettings::CsvEncoding;
						section->insert(name, v);
					}
				}

				xmlFree(attr);
			}
		}
#ifndef MVD_QTCORE_ONLY
		else if (type == "color")
		{
			xmlChar* txt = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if (txt)
			{
				QString n( (const char*) txt );
				xmlFree(txt);

				QColor col(n);
				if (col.isValid())
				{
					EncodedVariant v;
					v.data = QVariant(col);
					section->insert(name, v);
				}
			}
		}
#endif // MVD_QTCORE_ONLY

		cur = cur->next;

	} // while
}

//! \internal Returns a preferences value as a QVariant.
QVariant MvdSettings_P::getData(const QString& name, const QString& section)
{
	if (name.isEmpty())
		return QVariant();

	SectionTable::Iterator sectionIterator = data.find(section.isEmpty() ? 
		defaultSectionName : section);

	if (sectionIterator == data.end())
		return QVariant();

	Section& sec = sectionIterator.value();

	Section::Iterator itemIterator = sec.find(name);
	if (itemIterator == sec.end())
		return QVariant();

	return itemIterator.value().data;
}

//! \internal
void MvdSettings_P::setData(const QString& name, const QString& section, 
	const EncodedVariant& ev)
{
	if (name.isEmpty())
		return;

	QString actualSectionName = section.isEmpty() ? defaultSectionName : section;

	SectionTable::Iterator sectionIterator = data.find(actualSectionName);
	if (sectionIterator == data.end())
	{
		Section newSection;
		sectionIterator = data.insert(actualSectionName, newSection);
	}

	sectionIterator.value().insert(name, ev);
}


/************************************************************************
MvdSettings
*************************************************************************/

//! \internal
MvdSettings* MvdSettings::mInstance = 0;

//! \internal Private constructor.
MvdSettings::MvdSettings()
: d(new MvdSettings_P)
{
}

/*!
	Returns the application unique preferences instance.
*/
MvdSettings& MvdSettings::instance()
{
	if (mInstance == 0)
		mInstance = new MvdSettings;
	return *mInstance;
}

/*!
	Frees any allocated resources.
*/
MvdSettings::~MvdSettings()
{
	if (this == mInstance)
		delete d;
	else delete mInstance;
}

/*!
	Returns true if there are no preferences set.
*/
bool MvdSettings::isEmpty()
{
	return d->data.isEmpty();
}

/*!
	Sets the encoding type to be used with binary data.
	This only affects new items that are added (or modified)
	after calling this method.
	Older items will use the previously set encoding type!
*/
void MvdSettings::setBinaryEncodingType(BinaryEncoding e)
{
	d->encoding = e;
}

//! Returns the encoding type used to encode binary data.
MvdSettings::BinaryEncoding MvdSettings::binaryEncodingType() const
{
	return d->encoding;
}

//! Returns the last occurred error as an enum value.
MvdSettings::ErrorCode MvdSettings::lastErrorCode() const
{
	return d->lastErrorCode;
}

//! Returns the last occurred error as a human readable string.
QString MvdSettings::lastErrorString() const
{
	switch (d->lastErrorCode)
	{
	case NoError: return QCoreApplication::translate(
		"MvdSettings", "No error occurred.");
	case FileNotFoundError: return QCoreApplication::translate(
		"MvdSettings", "File not found.");
	case FileOpenError: return QCoreApplication::translate(
		"MvdSettings", "Unable to open preferences file.");
	case FileVersionError: return QCoreApplication::translate(
		"MvdSettings", "File was created with an incompatible MvdSettings version.");
	case ProductVersionError: return QCoreApplication::translate(
		"MvdSettings", "Preferences file created with an incompatible version of this application.");
	default: ;
	}

	return QCoreApplication::translate("MvdSettings", "Unknown error.");
}

/*!
	Attempts to load settings from a file. Old data is not cleared! 
	You will need to explicitly call clear() for this purpose.
	Returns false and sets the lastErrorCode property on error.
*/
bool MvdSettings::load(const QString& filename)
{
	if (!QFile::exists(filename))
	{
		d->lastErrorCode = FileNotFoundError;
		return false;
	}

	d->lastErrorCode = NoError;

	xmlDocPtr doc = xmlParseFile(filename.toAscii().data());
	if (!doc)
		return false;

	xmlNodePtr cur = xmlDocGetRootElement(doc);
	if (!cur)
	{
		xmlFreeDoc(doc);
		return false;
	}

	if (xmlStrcmp((cur)->name, (const xmlChar*) "xml-preferences"))
	{
		xmlFreeDoc(doc);
		return false;
	}

	xmlChar* attr = 0;
	QString version;

	attr = xmlGetProp(cur, (const xmlChar*) "xmlp-version");
	if (attr)
	{
		bool ok;
		quint8 xmlPrefV = QString((const char*) attr).toInt(&ok);
		xmlFree(attr);

		if (ok)
		{
			if (xmlPrefV != MvdSettings::version)
			{
				d->lastErrorCode = FileVersionError;
				return false;
			}
		}
	}

	// Check product version
	if (d->version != 0)
	{
		quint8 version = 0;
		xmlNodePtr childNode = cur->xmlChildrenNode;

		while (childNode)
		{
			if (!xmlStrcmp(childNode->name, (const xmlChar*) "product-info"))
			{
				attr = xmlGetProp(childNode, (const xmlChar*) "version");
				if (attr)
				{
					bool ok;
					version = QString((const char*) attr).toUShort(&ok);
					xmlFree(attr);
					if (!ok)
						version = 0;
					break;
				}
			}

			childNode = childNode->next;
		}

		if (!d->checkApplicationVersion(version))
		{
			xmlFreeDoc(doc);
			d->lastErrorCode = ProductVersionError;
			return false;
		}
	}

	xmlNodePtr childNode = cur->xmlChildrenNode;
	while (childNode)
	{
		if (xmlStrcmp(childNode->name, (const xmlChar*) "preferences"))
		{
			childNode = childNode->next;
			continue;
		}

		xmlNodePtr prefNode = childNode->xmlChildrenNode;
		while (prefNode)
		{
			if (xmlStrcmp(prefNode->name, (const xmlChar*) "section"))
			{
				prefNode = prefNode->next;
				continue;
			}

			attr = xmlGetProp(prefNode, (const xmlChar*) "name");

			if (!attr)
			{
				prefNode = prefNode->next;
				continue;
			}

			// Find or create the right section
			QString sectionName((const char*) attr);
			MvdSettings_P::SectionTable::Iterator sectionIterator
				= d->data.find(sectionName);

			if (sectionIterator == d->data.end())
			{
				// Create a new section
				MvdSettings_P::Section section;
				d->parseSection(doc, prefNode, &section);
				d->data.insert(sectionName, section);
			}
			else d->parseSection(doc, prefNode, &(sectionIterator.value()));

			xmlFree(attr);

			prefNode = prefNode->next;
		}

		childNode = childNode->next;
	}

	xmlFreeDoc(doc);
	return true;
}

/*!
	Attempts to write the settings to a file.
	Returns false and sets the lastErrorCode property on error.
*/
bool MvdSettings::save(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
	{
		d->lastErrorCode = file.exists() ? FileOpenError : FileNotFoundError;
		return false;
	}

	MvdXmlWriter out(&file);
	QHash<QString,QString> attrs;

	attrs.insert("xmlp-version", QString::number(MvdSettings::version));
	out.writeOpenTag("xml-preferences", attrs);
	attrs.clear();

	if (!d->company.isEmpty())
		attrs.insert("company", d->company);
	if (!d->product.isEmpty())
		attrs.insert("product", d->product);
	if (d->version != 0)
		attrs.insert("version", QString::number(d->version));

	if (!attrs.isEmpty())
	{
		out.writeAtomTag("product-info", attrs);
		attrs.clear();
	}

	out.writeOpenTag("preferences", attrs);

	for (MvdSettings_P::SectionTable::Iterator sectionIterator = d->data.begin();
		sectionIterator != d->data.end(); ++sectionIterator)
	{
		d->writeSection(&out, sectionIterator.key(), sectionIterator.value());
	}

	out.writeCloseTag("preferences");
	out.writeCloseTag("xml-preferences");

	return true;
}

/*!
	Returns the string value associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns QString() if no such value could be found.
*/
QString MvdSettings::getString(const QString& name, const QString& section)
{
	return d->getData(name, section).toString();
}

/*!
	Returns the string list associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns an empty list if no such value could be found.
*/
QStringList MvdSettings::getStringList(const QString& name, const QString& section)
{
	return d->getData(name, section).toStringList();
}

/*!
	Returns the boolean value associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Sets \p ok to false if no such value could be found and \p ok is not null.
*/
bool MvdSettings::getBool(const QString& name, const QString& section, bool* ok)
{
	QVariant v = d->getData(name, section);
	if (ok != 0)
		*ok = v.isValid() && v.type() == QVariant::Bool;
	return v.toBool();
}

/*!
	Returns the integer value associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Sets \p ok to false if no such value could be found and \p ok is not null.
*/
int MvdSettings::getInt(const QString& name, const QString& section, bool* ok)
{
	QVariant v = d->getData(name, section);
	if (ok != 0)
		*ok = v.isValid() && v.type() == QVariant::Int;
	return v.toInt();
}


/*!
	Returns the 64bit integer value associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Sets \p ok to false if no such value could be found and \p ok is not null.
*/
qint64 MvdSettings::getInt64(const QString& name, const QString& section, bool* ok)
{
	QVariant v = d->getData(name, section);
	if (ok != 0)
		*ok = v.isValid() && v.type() == QVariant::LongLong;
	return v.toLongLong();
}

/*!
	Returns the bit array associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns an empty array if no such value could be found
*/
QBitArray MvdSettings::getBitArray(const QString& name, const QString& section)
{
	return d->getData(name, section).toBitArray();
}

/*!
	Returns the byte array associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns an empty array if no such value could be found
*/
QByteArray MvdSettings::getByteArray(const QString& name, const QString& section)
{
	return d->getData(name, section).toByteArray();
}

/*!
	Returns the rectangle associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns an empty rectangle if no such value could be found
*/
QRect MvdSettings::getRect(const QString& name, const QString& section)
{
	return d->getData(name, section).toRect();
}

/*!
	Returns the point associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns an invalid point if no such value could be found
*/
QPoint MvdSettings::getPoint(const QString& name, const QString& section)
{
	return d->getData(name, section).toPoint();
}

/*!
	Returns the size associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns an invalid size if no such value could be found
*/
QSize MvdSettings::getSize(const QString& name, const QString& section)
{
	return d->getData(name, section).toSize();
}

#ifndef MVD_QTCORE_ONLY

/*!
	Returns the color associated to \p name in \p section or in the
	default section if the \p section name is empty.
	Returns a null color if no such value could be found
*/
QColor MvdSettings::getColor(const QString& name, const QString& section)
{
	return d->getData(name, section).value<QColor>();
}

#endif // MVD_QTCORE_ONLY

/*!
	Removes a section and all its data.
	Removes the default section if \p section is empty.
*/
void MvdSettings::clearSection(const QString& section)
{
	if (d->data.isEmpty())
		return;

	MvdSettings_P::SectionTable::Iterator sectionIterator
		= d->data.find(section.isEmpty() ? d->defaultSectionName : section);

	if (sectionIterator != d->data.end())
		d->data.erase(sectionIterator);
}

/*!
	Adds a string type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setString(const QString& name, const QString& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
	Adds a string list type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setStringList(const QString& name, const QStringList& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
Adds a boolean type value to \p section or to the default section if \p section is empty
and binds it to \p name.
*/
void MvdSettings::setBool(const QString& name, bool value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
	Adds an integer type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setInt(const QString& name, int value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
	Adds a 64bit integer type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setInt64(const QString& name, qint64 value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
	Adds a bit array integer type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setBitArray(const QString& name, const QBitArray& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	v.encoding = d->encoding;
	d->setData(name, section, v);
}

/*!
	Adds a byte array integer type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setByteArray(const QString& name, const QByteArray& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	v.encoding = d->encoding;
	d->setData(name, section, v);
}

/*!
	Adds a rectangle type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setRect(const QString& name, const QRect& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
	Adds a size type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setSize(const QString& name, const QSize& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

/*!
	Adds a point type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setPoint(const QString& name, const QPoint& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

#ifndef MVD_QTCORE_ONLY

/*!
	Adds a color type value to \p section or to the default section if \p section is empty
	and binds it to \p name.
*/
void MvdSettings::setColor(const QString& name, const QColor& value, const QString& section)
{
	MvdSettings_P::EncodedVariant v;
	v.data = QVariant(value);
	d->setData(name, section, v);
}

#endif // MVD_QTCORE_ONLY

/*!
	Clears all the data. This won't remove the name of the default section!
*/
void MvdSettings::clear()
{
	d->data.clear();
}

/*!
	Sets the product info.
	This is of no real use but if set it will be added at the top of the XML file.
*/
void MvdSettings::setProductInfo(const QString& company, const QString& product)
{
	d->company = company;
	d->product = product;
}

/*!
Sets the product version.
*/
void MvdSettings::setProductVersion(quint8 version)
{
	d->version = version;
}

/*!
	Sets the name of the default section if \p name is not empty.
*/
void MvdSettings::setDefaultSectionName(const QString& name)
{
	if (!name.isEmpty())
	{
		d->defaultSectionName = name;
	}
}

/*!
	Convenience method to access the MvdSettings singleton.
*/
MvdSettings& Movida::settings()
{
	return MvdSettings::instance();
}
