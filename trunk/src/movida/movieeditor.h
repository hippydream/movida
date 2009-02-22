/**************************************************************************
** Filename: movieeditor.h
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
        mvdid movieId() const;

        bool isModified() const;
        bool isValid() const;

        virtual QSize sizeHint() const;

public slots:
        void setPreviousEnabled(bool enabled);
        void setNextEnabled(bool enabled);
        bool storeMovie();

signals:
        void previousRequested();
        void nextRequested();

protected:
        virtual void closeEvent(QCloseEvent* e);

protected slots:
        virtual void validationStateChanged(MvdMPDialogPage* page);
        virtual void modifiedStateChanged(MvdMPDialogPage* page);

private slots:
        void cancelTriggered();
        void storeTriggered();
        void currentPageChanged(MvdMPDialogPage* page);

private:
        inline bool confirmDiscardMovie();

        QList<MvdMovieEditorPage*> mPages;
        MvdMovieCollection* mCollection;
        mvdid mMovieId;

        QPushButton* mPreviousButton;
        QPushButton* mNextButton;
        QPushButton* mOkButton;
        QPushButton* mCancelButton;
        QPushButton* mHelpButton;

        int mResetPageId;
        int mResetAllId;
        int mStoreChangesId;
};

#endif // MVD_MOVIEEDITOR_H
