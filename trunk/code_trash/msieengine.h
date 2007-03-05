/****************************************************************************
** Filename: msieengine.h
** Last updated [dd/mm/yyyy]: 22/04/2006
**
** Microsoft internet Explorer browser interface.
**
** Copyright (C) 2006 Angius Fabrizio. All rights reserved.
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
**********************************************************************/

#ifndef MOVIDA_MSIEBROWSERENGINE__H
#define MOVIDA_MSIEBROWSERENGINE__H

#include "browserengine.h"

#include <QFile>

class QTextBrowser;

class DefaultBrowserEngine : public BrowserEngine
{
	Q_OBJECT

public:
	MSIEBrowserEngine(QWidget* parent = 0);

	virtual void openUrl(const QString& url)
	{
		QFile f(url);
		f.open(QIODevice::ReadOnly);
		widget->setHtml(QString(f.readAll()));
	}

private:
	QTextBrowser* widget;
};

#endif // MOVIDA_MSIEBROWSERENGINE__H
