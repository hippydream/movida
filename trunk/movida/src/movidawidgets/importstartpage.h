/**************************************************************************
** Filename: importstartpage.h
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

#ifndef MVDW_IMPORTSTARTPAGE_H
#define MVDW_IMPORTSTARTPAGE_H

#include "importpage.h"
#include "searchengine.h"

class QLabel;
class QComboBox;
class QLineEdit;
class QPushButton;

class MvdwImportStartPage : public MvdwImportPage
{
	Q_OBJECT

public:
	MvdwImportStartPage(QWidget* parent = 0);

	int registerEngine(const MvdwSearchEngine& engine);

	int engine() const;
	QString query() const;

signals:
	void engineConfigurationRequest(int engine);

private slots:
	void engineChanged();
	void configButtonTriggered();

private:
	QList<MvdwSearchEngine> engines;

	QLabel* infoLabel;
	QComboBox* engineCombo;
	QLineEdit* queryInput;
	QPushButton* configButton;
};

#endif // MVDW_IMPORTSTARTPAGE_H

