/**************************************************************************
** Filename: movieeditor.h
** Revision: 3
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

#ifndef MVD_MOVIEEDITOR_H
#define MVD_MOVIEEDITOR_H

#include "multipagedialog.h"
#include "mvdcore/shareddata.h"
#include "mvdcore/movie.h"
#include <QList>

class MvdMovieCollection;
class MvdMovieEditorPage;
class QCloseEvent;
class QPushButton;

class MvdMovieEditor : public MvdMultiPageDialog
{
	Q_OBJECT
		
public:
	MvdMovieEditor(MvdMovieCollection* c, QWidget* parent = 0);
	
	bool setMovie(mvdid id, bool confirmIfModified = false);
	bool setMovies(const QList<mvdid>& movies, bool confirmIfModified = false);

public slots:
	void setPreviousEnabled(bool enabled);
	void setNextEnabled(bool enabled);

signals:
	void previousRequested();
	void nextRequested();

protected:
	virtual void closeEvent(QCloseEvent* e);

protected slots:
	virtual void validationStateChanged(MvdMPDialogPage* page);
	virtual void modifiedStateChanged(MvdMPDialogPage* page);
	virtual void advancedControlHandler(int control);

private slots:
	void cancelTriggered();
	void storeTriggered();
	bool storeMovie();
	void currentPageChanged(MvdMPDialogPage* page);

private:
	inline bool isModified() const;
	inline bool isValid() const;
	inline bool confirmDiscardMovie();

	QList<MvdMovieEditorPage*> mPages;
	MvdMovieCollection* mCollection;
	mvdid mMovieId;

	QPushButton* mPreviousButton;
	QPushButton* mNextButton;
	QPushButton* mOkButton;
	QPushButton* mHelpButton;

	int mResetPageId;
	int mResetAllId;
	int mStoreChangesId;
};

#endif // MVD_MOVIEEDITOR_H
