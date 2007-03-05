/**************************************************************************
** Filename: multipagedialog.h
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

#ifndef MVD_MULTIPAGEDIALOG_H
#define MVD_MULTIPAGEDIALOG_H

#include <QDialog>
#include <QMap>

#include "ui_multipagedialog.h"

class MvdMPDialogPage;

class MvdMultiPageDialog : public QDialog, private Ui::MvdMultiPageDialog
{
	Q_OBJECT

public:
	MvdMultiPageDialog(QWidget* parent = 0);

	virtual int addPage(MvdMPDialogPage* p);
	virtual void showPage(MvdMPDialogPage* p);

	virtual QDialogButtonBox* buttonBox() const;
};

#endif // MVD_MULTIPAGEDIALOG_H
