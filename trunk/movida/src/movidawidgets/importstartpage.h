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

#include "searchengine.h"

#include <QWizardPage>

class QLabel;
class QComboBox;
class QLineEdit;

class MvdwImportStartPage : public QWizardPage
{
	Q_OBJECT
	Q_PROPERTY(QString engine READ engine)

public:
	MvdwImportStartPage(const QList<MvdwSearchEngine>& engines, QWidget* parent = 0);

	void setInfoText(const QString& s);

	QString engine() const;

signals:
	void currentEngineChanged();

private slots:
	void engineChanged();

private:
	QList<MvdwSearchEngine> engines;

	QLabel* infoLabel;
	QComboBox* engineCombo;
	QLineEdit* queryInput;
};

#endif // MVDW_IMPORTSTARTPAGE_H

