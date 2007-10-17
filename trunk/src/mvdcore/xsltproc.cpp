/**************************************************************************
** Filename: xsltproc.cpp
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

#include "xsltproc.h"
#include <QFile>
#include <libxml/tree.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

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


/************************************************************************
MvdXsltProc_P
*************************************************************************/

//! \internal
class MvdXsltProc_P
{
public:
	MvdXsltProc_P();
	bool loadXslt(const QString& path);

	xsltStylesheetPtr stylesheet;
};

//! \internal
MvdXsltProc_P::MvdXsltProc_P()
: stylesheet(0)
{
	// init libxslt
	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 0;
}

//! \internal
bool MvdXsltProc_P::loadXslt(const QString& path)
{
	if (stylesheet)
	{
		xsltFreeStylesheet(stylesheet);
		stylesheet = 0;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QByteArray buffer = file.readAll();
	if (buffer.isEmpty())
		return false;

	xmlDocPtr doc = xmlParseMemory(buffer.data(), buffer.size());
	if (!doc)
		return false;

	stylesheet = xsltParseStylesheetDoc(doc);
	return stylesheet;
}


/************************************************************************
MvdXsltProc
*************************************************************************/

/*!
	Creates an invalid xsl processor. An XSL file needs to be set before
	you can use it.
*/
MvdXsltProc::MvdXsltProc()
{
}

//! Creates a xsl processor for the specified XSL file.
MvdXsltProc::MvdXsltProc(const QString& xslpath)
: d(new MvdXsltProc_P)
{
	d->loadXslt(xslpath);
}

//! Returns true if the processor is valid (a stylesheet has been correctly loaded).
bool MvdXsltProc::isOk() const
{
	return d->stylesheet;
}

//! Attempts to load a XSL stylesheet from a file.
bool MvdXsltProc::loadXslFile(const QString& xslpath)
{
	return d->loadXslt(xslpath);
}

//! Applies XSL transformations to the text using the previously loaded stylesheet.
QString MvdXsltProc::processText(const QString& txt)
{
	if (!isOk())
		return QString();

	QByteArray buffer = txt.toUtf8();
	if (buffer.isEmpty())
		return QString();

	xmlDocPtr doc = xmlParseMemory(buffer.data(), buffer.size());
	if (!doc)
		return QString();

	xmlDocPtr res = xsltApplyStylesheet(d->stylesheet, doc, NULL);
	if (!res)
		return QString();

	xmlChar* outString = NULL;
	int outStringLength = 0;

	xsltSaveResultToString(&outString, &outStringLength, res, d->stylesheet);
	if (!outString || !outStringLength)
		return QString();

	return QString::fromUtf8(reinterpret_cast<const char*>(outString), outStringLength);
}

//! Applies XSL transformations to the file using the previously loaded stylesheet.
QString MvdXsltProc::processFile(const QString& file)
{
	if (!isOk())
		return QString();

	xmlDocPtr doc = xmlParseFile(file.toLatin1().constData());
	if (!doc)
		return QString();

	xmlDocPtr res = xsltApplyStylesheet(d->stylesheet, doc, NULL);
	if (!res)
		return QString();

	xmlChar* outString = NULL;
	int outStringLength = 0;

	xsltSaveResultToString(&outString, &outStringLength, res, d->stylesheet);
	if (!outString || !outStringLength)
		return QString();

	return QString::fromUtf8(reinterpret_cast<const char*>(outString), outStringLength);
}
