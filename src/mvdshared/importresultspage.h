/**************************************************************************
** Filename: importresultspage.h
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

#ifndef MVD_IMPORTRESULTSPAGE_H
#define MVD_IMPORTRESULTSPAGE_H

#include "ui_importresultspage.h"
#include "sharedglobal.h"
#include "importexportpage.h"

class QLabel;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

class MvdImportResultsPage : public MvdImportExportPage
{
	Q_OBJECT
	Q_PROPERTY(int resultsCount READ resultsCount)
	Q_PROPERTY(int selectedResultsCount READ selectedResultsCount)

public:
	enum ItemType { StandardItem, SectionItem };

	MvdImportResultsPage(QWidget* parent = 0);

	int addMatch(QString title, const QString& year, const QString& notes = QString());
	void addSection(QString title, const QString& notes = QString());
	void addSubSection(QString title, const QString& notes = QString());

	void initializePage();
	void cleanupPage();

	QList<int> jobs() const;

	void showMessage(const QString& msg, MovidaShared::MessageType t);
	void setBusyStatus(bool busy);

	void setProgress(int v);
	void setProgressMaximum(int m);
	int progress() const;

	int resultsCount() const;
	int selectedResultsCount() const;

signals:
	void resultsCountChanged(int count = 0);
	void selectedResultsCountChanged(int count = 0);

private slots:
	void resultsSelectionChanged();
	void resultsCheckStateChanged();
	void ensureItemVisible();

private:
	Ui::MvdImportResultsPage ui;

	int matchId;
	int lastSelectedMatches;
	bool locked;

	QTreeWidgetItem* currentSection;

	int countMatches(int* selected = 0) const;
	int countSections(const QTreeWidgetItem* section = 0) const;
	void setLock(bool lock);
};

#endif // MVD_IMPORTRESULTSPAGE_H
