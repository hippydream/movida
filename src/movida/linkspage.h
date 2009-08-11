/**************************************************************************
** Filename: linkspage.h
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

#ifndef MVD_LINKSPAGE_H
#define MVD_LINKSPAGE_H

#include "ui_linkspage.h"

#include "movieeditorpage.h"

class QIcon;
class QTreeWidgetItem;

class MvdLinksPage : public MvdMovieEditorPage, private Ui::MvdLinksPage
{
    Q_OBJECT

public:
    MvdLinksPage(MvdMovieCollection *c, MvdMovieEditor *parent = 0);

    QString label();
    QIcon icon();

    void setMovieImpl(const MvdMovie &movie);

    bool store(MvdMovie &movie);

private slots:
    void imdbIdChanged(const QString &text);
    void openImdbPage();
    void contextMenuRequested(QTreeWidgetItem *item, int column);
    void itemChanged(QTreeWidgetItem *item, int column);
    void updateModifiedStatus();
    void deleteSelectedUrls();
    void openSelectedUrls();

private:
    inline void appendPlaceHolder();
    inline QList<MvdUrl> urls() const;

    QList<MvdUrl> mDefaultUrls;
    QString mDefaultImdbId;
    bool mLocked;
};

#endif // MVD_LINKSPAGE_H
