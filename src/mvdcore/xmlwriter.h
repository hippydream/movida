/**************************************************************************
** Filename: xmlwriter.h
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#include <QtCore/QHash>
#include <QtCore/QPair>

class QIODevice;
class QTextCodec;

class MVD_EXPORT MvdXmlWriter
{
public:
    enum Option { NoOptions = 0x0, WriteEncodingOption };
    Q_DECLARE_FLAGS(Options, Option)

    static const Options DefaultOptions;

    enum LineBreakType { UnixLineBreak, WindowsLineBreak, MacLineBreak };

    MvdXmlWriter(QIODevice * device, QTextCodec * codec = 0, Options = DefaultOptions);
    MvdXmlWriter(QString * string, QTextCodec * codec = 0, Options = DefaultOptions);
    virtual ~MvdXmlWriter();

    void writeString(const QString &string);
    void writeLine();

    void writeOpenTag(const QString &name, const MvdAttribute &a1, const MvdAttribute &a2 = MvdAttribute(), const MvdAttribute &a3 = MvdAttribute());
    void writeOpenTag(const QString &name, const MvdAttributeMap &attrs = MvdAttributeMap());
    void writeCloseTag(const QString &name);
    void writeAtomTag(const QString &name, const MvdAttribute &a1, const MvdAttribute &a2 = MvdAttribute(), const MvdAttribute &a3 = MvdAttribute());
    void writeAtomTag(const QString &name, const MvdAttributeMap &attrs = MvdAttributeMap());
    void writeTaggedString(const QString &name, const QString &string, const MvdAttribute &a1, const MvdAttribute &a2 = MvdAttribute(), const MvdAttribute &a3 = MvdAttribute());
    void writeTaggedString(const QString &name, const QString &string, const MvdAttributeMap &attrs = MvdAttributeMap());
    void writeCDataString(const QString &name, const QString &string, const MvdAttribute &a1, const MvdAttribute &a2 = MvdAttribute(), const MvdAttribute &a3 = MvdAttribute());
    void writeCDataString(const QString &name, const QString &string, const MvdAttributeMap &attrs = MvdAttributeMap());

    void writeComment(const QString &comment);
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
    class Private;
    Private *d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdXmlWriter::Options)

#endif // MVD_XMLWRITER_H
