/**************************************************************************
** Filename: importdialog.h
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

#include "widgetsglobal.h"
#include "searchengine.h"

#include <QDialog>
#include <QUuid>
#include <QHash>
#include <QVariant>
#include <QPushButton>
#include <QWizard>
#include <QStringList>

class MvdwLabelAnimator;
class QPushButton;

class MVDW_EXPORT MvdwImportDialog : public QWizard
{
	Q_OBJECT

public:
	enum ResponseStatus
	{
		SingleMatchResponse,
		MultipleMatchesResponse,
		ErrorResponse
	};

	MvdwImportDialog(const QList<MvdwSearchEngine>& engines, QWidget* parent = 0);

	void registerResponseHandler(QObject* handler, const char* member);
	void addMatch(const QString& title);

	// Reimplements QDialog's accept() slot.
	// This is called when the user hits the accept button (aka "Import")
	void accept();
	
// 	QWidget* startPage();
// 	void setStatus(const QString& s);
// 
// 	QUuid addSearchResult(const QString& displayString);
// 	void addMovieData(const QUuid& id, const QHash<QString,QVariant>& movieData);

public slots:
// 	void showStartPage();
// 	void showImportPage();
// 
// 	void setBusyStatus(bool busy);
// 	void setSearchButtonEnabled(bool enabled);
// 	void setBackButtonEnabled(bool enabled);
// 	void setImportButtonEnabled(bool enabled);
// 
// 	void clearResults();

signals:

// 	void searchTriggered();
// 	void importTriggered();
// 	void showStartPageTriggered();
// 	void movieRequired(const QUuid& id);

private slots:
// 	void resultsStatusChanged();
// 	void resultsSelectionChanged();

private:
	int resultsPageId;
	int startPageId;
// 	QPushButton* searchButton;
// 	QPushButton* backButton;
// 	QPushButton* importButton;
// 	MvdwLabelAnimator* labelAnimator;
// 	QHash<QUuid, QHash<QString,QVariant> > movies;
};
