/**************************************************************************
** Filename: passwordprompt.h
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

#ifndef MVD_PASSWORDPROMPT_H
#define MVD_PASSWORDPROMPT_H

#include "ui_passwordprompt.h"

#include <QDialog>

class QString;

class MvdPasswordPrompt : public QDialog, private Ui::MvdPasswordPrompt
{
    Q_OBJECT

public:
	enum ExitStatus { ExitOk, ExitSkip, ExitSkipAll };

	MvdPasswordPrompt(const QString& file, QWidget* parent = 0);

	QString password() const;
	ExitStatus exitStatus() const;

private slots:
	void ok();
	void skip();
	void skipAll();
	
private:
	ExitStatus mStatus;
};

#endif // MVD_PASSWORDPROMPT_H
