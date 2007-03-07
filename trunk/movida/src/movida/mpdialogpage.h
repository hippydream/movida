/**************************************************************************
** Filename: complexpage.h
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

#ifndef MVD_MPDIALOGPAGE_H
#define MVD_MPDIALOGPAGE_H

#include <QWidget>

class QIcon;

class MvdMPDialogPage : public QWidget
{
	Q_OBJECT

public:
	MvdMPDialogPage(QWidget* parent = 0) : QWidget(parent) {};

	virtual void reset() = 0;
	virtual QString label() = 0;
	virtual QIcon icon() = 0;

signals:
	void externalActionTriggered(const QString& id, const QVariant& data);
};

#endif // MVD_MPDIALOGPAGE_H
