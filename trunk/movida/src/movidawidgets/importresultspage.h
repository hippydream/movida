/**************************************************************************
** Filename: importresultspage.h
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

#ifndef MVDW_IMPORTRESULTSPAGE_H
#define MVDW_IMPORTRESULTSPAGE_H

#include "importpage.h"

class QTreeWidget;
class QLabel;
class QTreeWidgetItem;

class MvdwImportResultsPage : public MvdwImportPage
{
	Q_OBJECT

public:
	enum ItemType { StandardItem, SectionItem };

	MvdwImportResultsPage(QWidget* parent = 0);

	int addMatch(const QString& title, const QString& year, const QString& notes = QString());
	void addSection(const QString& title, const QString& notes = QString());
	void addSubSection(const QString& title, const QString& notes = QString());

	void initializePage();
	void cleanupPage();

	int countMatches() const;
	QList<int> jobs() const;

	void showMessage(const QString& msg, MvdwImportPage::MessageType t = MvdwImportPage::InfoMessage);
	void setBusyStatus(bool busy);

private slots:
	void resultsSelectionChanged();
	void resultsCheckStateChanged();

private:
	QTreeWidget* results;
	QLabel* infoLabel;
	int matchId;
	bool locked;

	int countSections(const QTreeWidgetItem* section = 0) const;
	void setLock(bool lock);
};

#endif // MVDW_IMPORTRESULTSPAGE_H
