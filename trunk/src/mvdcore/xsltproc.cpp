/**************************************************************************
** Filename: xsltproc.cpp
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

#include "xsltproc.h"

#include "logger.h"

#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QTextStream>

#include <libxml/tree.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

using namespace Movida;

/*!
    \class MvdXsltProc xsltproc.h
    \ingroup MvdCore

    \brief Simple interface to the libxslt library.
*/

/*
    From http://xmlsoft.org/XSLT/API.html:

    <BLOCKQUOTE>
    Basically doing an XSLT transformation can be done in a few steps:

    1. configure the parser for XSLT:
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;
    2. parse the stylesheet with xsltParseStylesheetFile()
    3. parse the document with xmlParseFile()
    4. apply the stylesheet using xsltApplyStylesheet()
    5. save the result using xsltSaveResultToFile() if needed set xmlIndentTreeOutput to 1

    Steps 2,3, and 5 will probably need to be changed depending on you processing needs and
    environment for example if reading/saving from/to memory, or if you want to apply
    XInclude processing to the stylesheet or input documents.
    </BLOCKQUOTE>
 */


namespace MvdXslt {
int writeToTextStream(void *context, const char *buffer, int len)
{
    QTextStream *stream = static_cast<QTextStream *>(context);
    QString s = QString::fromUtf8(buffer, len);

    *stream << s;
    return len;
}

int closeTextStream(void *context)
{
    QTextStream *s = static_cast<QTextStream *>(context);

    s->flush();
    return 0;
}

}


/************************************************************************
   MvdXsltProc::Private
 *************************************************************************/

//! \internal
class MvdXsltProc::Private
{
public:
    Private();
    bool loadXslt(const QString &path);
    xmlDocPtr applyStylesheet(xmlDocPtr doc,
    const MvdXsltProc::ParameterList &params) const;

    xsltStylesheetPtr stylesheet;
};

//! \internal
MvdXsltProc::Private::Private() :
    stylesheet(0)
{
    // init libxslt
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 0;
}

//! \internal
bool MvdXsltProc::Private::loadXslt(const QString &path)
{
    if (stylesheet) {
        xsltFreeStylesheet(stylesheet);
        stylesheet = 0;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        eLog() << "MvdXsltProc: Failed to open " << path << " for reading (" << file.errorString() << ").";
        return false;
    }

    QByteArray buffer = file.readAll();
    if (buffer.isEmpty()) {
        eLog() << "MvdXsltProc: Failed to read " << path << ".";
        return false;
    }

    xmlDocPtr doc = xmlParseMemory(buffer.data(), buffer.size());
    if (!doc) {
        eLog() << "MvdXsltProc: Invalid XML file: " << path;
        return false;
    }

    stylesheet = xsltParseStylesheetDoc(doc);
    if (!stylesheet)
        eLog() << "MvdXsltProc: Invalid XSL file: " << path;
    return stylesheet;
}

xmlDocPtr MvdXsltProc::Private::applyStylesheet(xmlDocPtr doc,
    const MvdXsltProc::ParameterList &params) const
{
    /*
        I know it looks silly, but libxslt expects the parameter array
        to consist at most of 16 name-value pairs + a final null, so
        we need a fixed size array with 17 positions.
     */
    static const int MaxXsltParameters = 16;
    const char *c_params[MaxXsltParameters + 1];

    c_params[0] = NULL;

    int paramCount = 0;
    ParameterList::ConstIterator it = params.constBegin();
    while (it != params.constEnd() && paramCount != MaxXsltParameters) {
        const QString &k = it.key();
        const QString &v = it.value();

        if (k.isEmpty())
            continue;

        char *c_k = qstrdup(k.toUtf8().data());
        char *c_v = qstrdup(v.toUtf8().data());

        c_params[paramCount] = c_k;
        c_params[paramCount + 1] = c_v;
        paramCount += 2;
    }

    xmlDocPtr tDoc = xsltApplyStylesheet(stylesheet, doc, c_params);

    if (paramCount) {
        for (int i = 0; i < paramCount; ++i)
            delete c_params[i];
    }

    return tDoc;
}

/************************************************************************
   MvdXsltProc
 *************************************************************************/

/*!
    Creates an invalid xsl processor. An XSL file needs to be set before
    you can use it.
*/
MvdXsltProc::MvdXsltProc()
{ }

//! Creates a xsl processor for the specified XSL file.
MvdXsltProc::MvdXsltProc(const QString &xslpath) :
    d(new Private)
{
    d->loadXslt(xslpath);
}

//! Returns true if the processor is valid (a stylesheet has been correctly loaded).
bool MvdXsltProc::isOk() const
{
    return d->stylesheet;
}

//! Attempts to load a XSL stylesheet from a file.
bool MvdXsltProc::loadXslFile(const QString &xslpath)
{
    return d->loadXslt(xslpath);
}

//! Applies XSL transformations to the text using the previously loaded stylesheet.
QString MvdXsltProc::processText(const QString &txt, const ParameterList &params)
{
    if (txt.isEmpty() || !isOk())
        return QString();

    QByteArray buffer = txt.toUtf8();

    xmlDocPtr doc = xmlParseMemory(buffer.data(), buffer.size());
    if (!doc)
        return QString();

    xmlDocPtr res = d->applyStylesheet(doc, params);
    if (!res)
        return QString();

    xmlChar *outString = NULL;
    int outStringLength = 0;

    xsltSaveResultToString(&outString, &outStringLength, res, d->stylesheet);
    if (!outString || !outStringLength)
        return QString();

    return QString::fromUtf8(reinterpret_cast<const char *>(outString), outStringLength);
}

//! Applies XSL transformations to the file using the previously loaded stylesheet.
QString MvdXsltProc::processFile(const QString &file, const ParameterList &params)
{
    if (!QFile::exists(file) || !isOk())
        return QString();

    xmlDocPtr doc = xmlParseFile(file.toLatin1().constData());
    if (!doc)
        return QString();

    xmlDocPtr res = d->applyStylesheet(doc, params);
    if (!res)
        return QString();

    xmlChar *outString = NULL;
    int outStringLength = 0;

    xsltSaveResultToString(&outString, &outStringLength, res, d->stylesheet);
    if (!outString || !outStringLength)
        return QString();

    return QString::fromUtf8(reinterpret_cast<const char *>(outString), outStringLength);
}

//! Applies XSL transformations to the text using the previously loaded stylesheet and writes the result to a device.
bool MvdXsltProc::processTextToDevice(const QString &txt, QIODevice *dev,
    const ParameterList &params)
{
    if (txt.isEmpty() || !isOk())
        return false;

    QByteArray buffer = txt.toUtf8();

    xmlDocPtr doc = xmlParseMemory(buffer.data(), buffer.size());
    if (!doc)
        return false;

    xmlDocPtr res = d->applyStylesheet(doc, params);
    if (!res)
        return false;

    QTextStream stream(dev);

    xmlOutputBufferPtr outp = xmlOutputBufferCreateIO(
        MvdXslt::writeToTextStream, (xmlOutputCloseCallback) MvdXslt::closeTextStream, &stream, 0);
    outp->written = 0;

    xsltSaveResultTo(outp, res, d->stylesheet);
    xmlOutputBufferFlush(outp);
    xmlFreeDoc(res);

    return true;
}

//! Applies XSL transformations to the file using the previously loaded stylesheet and writes the result to a device.
bool MvdXsltProc::processFileToDevice(const QString &file, QIODevice *dev,
    const ParameterList &params)
{
    if (!QFile::exists(file) || !isOk() || !dev)
        return false;

    xmlDocPtr doc = xmlParseFile(file.toLatin1().constData());
    if (!doc)
        return false;

    xmlDocPtr res = d->applyStylesheet(doc, params);
    if (!res)
        return false;

    QTextStream *stream = new QTextStream(dev);

    xmlOutputBufferPtr outp = xmlOutputBufferCreateIO(
        MvdXslt::writeToTextStream, (xmlOutputCloseCallback) MvdXslt::closeTextStream, &stream, 0);
    outp->written = 0;

    xsltSaveResultTo(outp, res, d->stylesheet);
    xmlOutputBufferFlush(outp);
    xmlFreeDoc(res);

    delete stream;
    return true;
}
