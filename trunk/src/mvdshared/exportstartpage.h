/**************************************************************************
** Filename: exportstartpage.h
**
** Copyright (C) 2007-2008 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_EXPORTSTARTPAGE_H
#define MVD_EXPORTSTARTPAGE_H

#include "sharedglobal.h"
#include "importexportpage.h"
#include "exportengine.h"

class MvdActionLabel;
class QLabel;
class QComboBox;
class QPushButton;
class QRadioButton;

class MvdExportStartPage : public MvdImportExportPage
{
	Q_OBJECT

public:
	MvdExportStartPage(QWidget* parent = 0);

	virtual void initializePage();
	void reset();

	int registerEngine(const MvdExportEngine& engine);

	int engine() const;

signals:
	void engineConfigurationRequest(int engine);

private slots:
	void engineChanged();
	void controlTriggered(int);

private:
	QLabel* mInfoLabel;
	QComboBox* mEngineCombo;
	QRadioButton* mExportSelectedButton;
	QRadioButton* mExportAllButton;
	MvdActionLabel* mControls;
	int mConfigureEngineId;
	int mConfigurePluginId;
	QList<MvdExportEngine> mEngines;
};

#endif // MVD_EXPORTSTARTPAGE_H
