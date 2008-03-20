/**************************************************************************
** Filename: xmlwriter.h
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

#ifndef MVD_XMLWRITER_H
#define MVD_XMLWRITER_H

#include "global.h"
#include <QHash>
#include <QPair>

class QIODevice;
class QTextCodec;

class MvdXmlWriter_P;

class MVD_EXPORT MvdXmlWriter
{
public:
	// Fix a bug with GCC not able to parse the template class default parameters
	typedef QHash<QString,QString> AttributeMap;
	typedef QPair<QString,QString> Attribute;

	enum Option { NoOptions = 0x0, WriteEncodingOption };
	Q_DECLARE_FLAGS(Options, Option)

	enum LineBreakType { UnixLineBreak, WindowsLineBreak, MacLineBreak };

	MvdXmlWriter(QIODevice* device, QTextCodec* codec = 0, Options = WriteEncodingOption);
	MvdXmlWriter(QString* string, QTextCodec* codec = 0, Options = WriteEncodingOption);
	virtual ~MvdXmlWriter();

	void writeString(const QString& string);
	void writeLine();

	void writeOpenTag(const QString& name, const Attribute& a1, const Attribute& a2 = Attribute(), const Attribute& a3 = Attribute());
	void writeOpenTag(const QString& name, const AttributeMap& attrs = AttributeMap());
	void writeCloseTag(const QString& name);
	void writeAtomTag(const QString& name, const Attribute& a1, const Attribute& a2 = Attribute(), const Attribute& a3 = Attribute());
	void writeAtomTag(const QString& name, const AttributeMap& attrs = AttributeMap());
	void writeTaggedString(const QString& name, const QString& string, const Attribute& a1, const Attribute& a2 = Attribute(), const Attribute& a3 = Attribute());
	void writeTaggedString(const QString& name, const QString& string, const AttributeMap& attrs = AttributeMap());

	void writeComment(const QString& comment);
	void startComment();
	void endComment();

	LineBreakType lineBreakType() const;
	void setLineBreakType(LineBreakType type);

	bool pauseIndent() const;
	void setPauseIndent(bool pause);

	void writeCurrentIndent();

	int indentType() const;
	void setIndentType(int spaces);

	bool autoNewLine() const;
	void setAutoNewLine(bool on);

	bool skipEmptyTags() const;
	void setSkipEmptyTags(bool skip);

	bool skipEmptyAttributes() const;
	void setSkipEmptyAttributes(bool skip);

private:
	MvdXmlWriter_P* d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdXmlWriter::Options)

#endif // MVD_XMLWRITER_H
