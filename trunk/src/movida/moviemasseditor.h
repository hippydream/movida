/**************************************************************************
** Filename: moviemasseditor.h
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_MOVIEMASSEDITOR_H
#define MVD_MOVIEMASSEDITOR_H

#include "movieeditor.h"
#include "ui_moviemasseditor.h"

#include "mvdcore/movie.h"

#include <QtCore/QList>
#include <QtGui/QDialog>

class MvdMovieCollection;
class QCloseEvent;

class MvdMovieMassEditor : public MvdMovieEditor
{
    Q_OBJECT

public:
    MvdMovieMassEditor(MvdMovieCollection *c, QWidget *parent = 0);
    virtual ~MvdMovieMassEditor();

    bool setMovies(const QList<mvdid> &ids, bool confirmIfModified = false);
    QList<mvdid> movieIds() const;

    //bool isModified() const;
    //bool isValid() const;

public slots:
    bool storeMovies();

protected:
    virtual void loadPages();
    virtual void closeEvent(QCloseEvent *e);

protected slots:
    virtual void validationStateChanged(MvdMPDialogPage *page);
    virtual void modifiedStateChanged(MvdMPDialogPage *page);

private:
    class Private;
    Private* d;
};

#endif // MVD_MOVIEMASSEDITOR_H
