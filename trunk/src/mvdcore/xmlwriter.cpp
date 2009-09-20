/**************************************************************************
** Filename: xmlwriter.cpp
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

#include "xmlwriter.h"

#include <QtCore/QIODevice>
#include <QtCore/QRegExp>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>


/*!
    \class MvdXmlWriter xmlwriter.h
    \ingroup MvdCore

    \brief Provides a convenient interface for writing XML files.
*/

const MvdXmlWriter::Options MvdXmlWriter::DefaultOptions = WriteEncodingOption;


/************************************************************************
    MvdXmlWriter::Private
 *************************************************************************/

//! \internal
class MvdXmlWriter::Private
{
public:
    Private(QIODevice *device, QTextCodec *codec,
        MvdXmlWriter::Options options);
    Private(QString *string, QTextCodec *codec,
        MvdXmlWriter::Options options);
    ~Private();

    QString escape(const QString &str) const;
    QString cdata(const QString &str) const;
    QString openTag(const QString &tag, const QHash<QString, QString> &attributes);

    QTextStream *stream;
    QString lineBreak;
    QString indentString;
    bool autoLineBreak;
    bool lineStart;
    bool pauseIndent;
    int indentLevel;
    bool skipEmptyTags;
    bool skipEmptyAttributes;
};

//! \internal
MvdXmlWriter::Private::Private(QIODevice *device, QTextCodec *codec,
    MvdXmlWriter::Options options)
{
    Q_ASSERT(device);
    stream = new QTextStream(device);
    if (codec == 0)
        stream->setCodec("UTF-8");
    else
        stream->setCodec(codec);

    indentString = "\t";
    autoLineBreak = true;
    lineBreak = "\r\n";
    indentLevel = 0;
    pauseIndent = false;
    skipEmptyTags = false;
    skipEmptyAttributes = false;

    // <?xml version="1.0" encoding="SELECTED_ENCODING"?>
    if (options.testFlag(MvdXmlWriter::WriteEncodingOption))
        (*stream) << "<?xml version=\"1.0\" encoding=\"" <<
        escape(QString(stream->codec()->name())) << "\"?>" <<
        lineBreak << lineBreak;
}

//! \internal
MvdXmlWriter::Private::Private(QString *string, QTextCodec *codec,
    MvdXmlWriter::Options options)
{
    Q_ASSERT(string);
    stream = new QTextStream(string);
    if (codec == 0)
        stream->setCodec("UTF-8");
    else
        stream->setCodec(codec);

    indentString = "\t";
    autoLineBreak = true;
    lineBreak = "\r\n";
    indentLevel = 0;
    pauseIndent = false;
    skipEmptyTags = false;
    skipEmptyAttributes = false;

    // <?xml version="1.0" encoding="SELECTED_ENCODING"?>
    if (options.testFlag(MvdXmlWriter::WriteEncodingOption))
        (*stream) << "<?xml version=\"1.0\" encoding=\"" <<
        escape(QString(stream->codec()->name())) << "\"?>" <<
        lineBreak << lineBreak;
}

//! \internal
MvdXmlWriter::Private::~Private()
{
    stream->flush();
    delete stream;
}

//! \internal
QString MvdXmlWriter::Private::escape(const QString &string) const
{
    QString s = string;

    s.replace("&", "&amp;");
    s.replace(">", "&gt;");
    s.replace("<", "&lt;");
    s.replace("\"", "&quot;");
    s.replace("\'", "&apos;");
    return s;
}

//! \internal
QString MvdXmlWriter::Private::cdata(const QString &string) const
{
    QString s = string;

    s.replace("<![CDATA[", "]]>&lt;![CDATA[<![CDATA[");
    s.replace("]]>", "]]>]]&gt;<![CDATA[");
    return s.prepend("<![CDATA[").append("]]>");
}

//! \internal
QString MvdXmlWriter::Private::openTag(const QString &tag, const QHash<QString, QString> &attributes)
{
    QString out = "<";

    out.append(escape(tag));

    for (QHash<QString, QString>::ConstIterator itr = attributes.constBegin();
         itr != attributes.constEnd(); ++itr) {
        if (skipEmptyAttributes && itr.value().isEmpty())
            continue;

        out.append(QString(" %1=\"%2\"").arg(escape(itr.key()))
                .arg(escape(itr.value())));
    }

    out.append(">");

    return out;
}


//////////////////////////////////////////////////////////////////////////


/*!
    Builds a new XML writer for \p device using \p codec to encode
    text. A first line with XML encoding type is written if \p writeEncoding is true.
    UTF-8 encoding is used if \p codec is 0.
    Indenting uses the ASCII horizontal tab char (\t) by default.
    CRLF is automatically added after each tag. This behavior can be changed by
    calling \link setAutoNewLine(bool)
*/
MvdXmlWriter::MvdXmlWriter(QIODevice *device, QTextCodec *codec, Options options) :
    d(new Private(device, codec, options))
{ }

/*!
    Convenience constructor, builds a new XML writer that writes to a string.
*/
MvdXmlWriter::MvdXmlWriter(QString *string, QTextCodec *codec, Options options) :
    d(new Private(string, codec, options))
{ }

/*!
    Flushes any remaining buffered data and destroys this object.
*/
MvdXmlWriter::~MvdXmlWriter()
{
    delete d;
}

/*!
    Returns the type of line breaks used by this writer.
*/
MvdXmlWriter::LineBreakType MvdXmlWriter::lineBreakType() const
{
    return (d->lineBreak == "\r" ? MacLineBreak : d->lineBreak == "\n" ?
            UnixLineBreak : WindowsLineBreak);
}

/*!
    Sets the type of line breaks the writer should use to either Unix (LF only),
    Windows (CRLF), or Mac (CR only) style.
*/
void MvdXmlWriter::setLineBreakType(LineBreakType type)
{
    switch (type) {
        case MacLineBreak:
            d->lineBreak = "\r"; break;

        case WindowsLineBreak:
            d->lineBreak = "\r\n"; break;

        case UnixLineBreak:
            d->lineBreak = "\n";

        default:
            ;
    }
}

/*!
    Writes a string to the stream encoding not allowed chars.
*/
void MvdXmlWriter::writeString(const QString &string)
{
    (*d->stream) << d->escape(string);
}

//! Convenience method.
void MvdXmlWriter::writeOpenTag(const QString &name, const MvdAttribute &a1, const MvdAttribute &a2, const MvdAttribute &a3)
{
    MvdAttributeMap a;

    if (!a1.first.isEmpty())
        a.insert(a1.first, a1.second);
    if (!a2.first.isEmpty())
        a.insert(a2.first, a2.second);
    if (!a3.first.isEmpty())
        a.insert(a3.first, a3.second);
    writeOpenTag(name, a);
}

/*!
    Writes an opening tag named \p name containing attributes from the
    \p attrs hash.
    Example: \verbatim <itemName attr1="value1"> \endverbatim
*/
void MvdXmlWriter::writeOpenTag(const QString &name, const MvdAttributeMap &attrs)
{
    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    d->indentLevel++;

    (*d->stream) << d->openTag(name, attrs);

    if (d->autoLineBreak)
        (*d->stream) << d->lineBreak;
}

/*!
    Writes a closing tag named \p name.
    Example: \verbatim </itemName> \endverbatim
*/
void MvdXmlWriter::writeCloseTag(const QString &name)
{
    d->indentLevel--;

    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    (*d->stream) << "</" << d->escape(name) << ">";
    if (d->autoLineBreak)
        (*d->stream) << d->lineBreak;
}

//! Convenience method.
void MvdXmlWriter::writeAtomTag(const QString &name, const MvdAttribute &a1, const MvdAttribute &a2, const MvdAttribute &a3)
{
    MvdAttributeMap a;

    if (!a1.first.isEmpty())
        a.insert(a1.first, a1.second);
    if (!a2.first.isEmpty())
        a.insert(a2.first, a2.second);
    if (!a3.first.isEmpty())
        a.insert(a3.first, a3.second);
    writeAtomTag(name, a);
}

/*!
    Writes an atom named \p name with attributes from hash \p attrs.
    Example: \verbatim <itemName attr1="value1"/> \endverbatim
*/
void MvdXmlWriter::writeAtomTag(const QString &name, const MvdAttributeMap &attrs)
{
    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    QString atom = d->openTag(name, attrs);
    atom.truncate(atom.length() - 1);
    (*d->stream) << atom << "/>";

    if (d->autoLineBreak)
        (*d->stream) << d->lineBreak;
}

//! Convenience method.
void MvdXmlWriter::writeTaggedString(const QString &name, const QString &string,
    const MvdAttribute &a1, const MvdAttribute &a2, const MvdAttribute &a3)
{
    MvdAttributeMap a;

    if (!a1.first.isEmpty())
        a.insert(a1.first, a1.second);
    if (!a2.first.isEmpty())
        a.insert(a2.first, a2.second);
    if (!a3.first.isEmpty())
        a.insert(a3.first, a3.second);
    writeTaggedString(name, string, a);
}

/*!
    Writes an opening tag, a string and a closing tag.
    Example:
    \verbatim <itemName attr1="value1">Some funny text</itemName> \endverbatim
*/
void MvdXmlWriter::writeTaggedString(const QString &name, const QString &string,
    const MvdAttributeMap &attrs)
{
    if (d->skipEmptyTags && string.isEmpty())
        return;

    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    (*d->stream) << d->openTag(name, attrs) << d->escape(string) << "</" <<
    d->escape(name) << ">";

    if (d->autoLineBreak)
        (*d->stream) << d->lineBreak;
}

//! Convenience method.
void MvdXmlWriter::writeCDataString(const QString &name, const QString &string,
    const MvdAttribute &a1, const MvdAttribute &a2, const MvdAttribute &a3)
{
    MvdAttributeMap a;

    if (!a1.first.isEmpty())
        a.insert(a1.first, a1.second);
    if (!a2.first.isEmpty())
        a.insert(a2.first, a2.second);
    if (!a3.first.isEmpty())
        a.insert(a3.first, a3.second);
    writeCDataString(name, string, a);
}

/*!
    Writes an opening tag, a string inside a CDATA section and a closing tag.
    Example:
    \verbatim <itemName attr1="value1"><![CDATA[Some funny text]]></itemName> \endverbatim
*/
void MvdXmlWriter::writeCDataString(const QString &name, const QString &string,
    const MvdAttributeMap &attrs)
{
    if (d->skipEmptyTags && string.isEmpty())
        return;

    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    (*d->stream) << d->openTag(name, attrs) << d->cdata(string) << "</" <<
    d->escape(name) << ">";

    if (d->autoLineBreak)
        (*d->stream) << d->lineBreak;
}

/*!
    Writes a comment tag.
    Example: \verbatim <!-- comment string --> \endverbatim
*/
void MvdXmlWriter::writeComment(const QString &comment)
{
    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    // We do not want the comments to end before WE want it ;)
    QString com(comment);
    com.replace(QString("-->"), QString("==>"));

    (*d->stream) << "<!-- " << com << " -->";

    if (d->autoLineBreak)
        (*d->stream) << d->lineBreak;
}

/*
    Starts a comment tag. No line break is added.
 */
void MvdXmlWriter::startComment()
{
    if (!d->pauseIndent)
        for (int i = 0; i < d->indentLevel; ++i)
            (*d->stream) << d->indentString;

    (*d->stream) << "<!-- ";
}

/*!
    Closes a comment tag. No line break or indent is added.
*/
void MvdXmlWriter::endComment()
{
    (*d->stream) << " -->";
}

/*!
    Outputs a line break using the style set with
    setLineBreakType(LineBreakType)
*/
void MvdXmlWriter::writeLine()
{
    (*d->stream) << d->lineBreak;
}

/*!
    Writes out some indent (level is determined by previous tags).
    Ignores any previous call to pauseIndent(bool).
*/
void MvdXmlWriter::writeCurrentIndent()
{
    for (int i = 0; i < d->indentLevel; ++i)
        (*d->stream) << d->indentString;
}

/*!
    Returns true if indent has been previously suspended by calling
    setPauseIndent(bool)
*/
bool MvdXmlWriter::pauseIndent() const
{
    return d->pauseIndent;
}

/*!
    Stops writing out indents.
    Indentation level is still recorded as tags get opened or closed.
*/
void MvdXmlWriter::setPauseIndent(bool pause)
{
    d->pauseIndent = pause;
}

/*!
    Returns the number of space chars used for indenting or -1 if the
    ASCII HTAB char is used.
*/
int MvdXmlWriter::indentType() const
{
    return d->indentString == "\t" ? -1 : d->indentString.length();
}

/*!
    Sets the number of spaces to use for indent.
    Uses ASCII HTAB "\t" tabs if \p spaces is negative.
*/
void MvdXmlWriter::setIndentType(int spaces)
{
    if (spaces < 0)
        d->indentString = "\t";
    else {
        d->indentString.clear();
        d->indentString.fill(' ', spaces);
    }
}

/*!
    Return true if a line break is automatically added after each tag.
*/
bool MvdXmlWriter::autoNewLine() const
{
    return d->autoLineBreak;
}

/*!
    Automatically adds a line break after each tag if \p on is true.
*/
void MvdXmlWriter::setAutoNewLine(bool on)
{
    d->autoLineBreak = on;
}

/*!
    Returns true if empty tags should not be written (i.e. no tag is written if you call
    writeTaggedString() passing an empty string). Default behavior is to write empty tags too.
    Only affects tags added by
    writeTaggedString(const QString&, const QString&, const AttributeMap&)
*/
bool MvdXmlWriter::skipEmptyTags() const
{
    return d->skipEmptyTags;
}

/*!
    Sets whether empty tags should be written or not.
*/
void MvdXmlWriter::setSkipEmptyTags(bool skip)
{
    d->skipEmptyTags = skip;
}

/*!
    Returns true if empty attributes should not be written.
    Default behavior is to write empty attributes too.
*/
bool MvdXmlWriter::skipEmptyAttributes() const
{
    return d->skipEmptyAttributes;
}

/*!
    Sets whether empty attributes should be written or not.
*/
void MvdXmlWriter::setSkipEmptyAttributes(bool skip)
{
    d->skipEmptyAttributes = skip;
}
