/**************************************************************************
** Filename: browserview.h
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

#ifndef MVD_BROWSERVIEW_H
#define MVD_BROWSERVIEW_H

#include "sharedglobal.h"

#include "mvdcore/global.h"

#include <QtCore/QList>
#include <QtCore/QUrl>
#include <QtGui/QWidget>

class MvdMovieCollection;
class MvdMovieData;

class QAction;
class QWebFrame;
class QWebPage;
class QWebView;

class MVD_EXPORT_SHARED MvdBrowserView : public QWidget
{
    Q_OBJECT

public:
    MvdBrowserView(QWidget *parent = 0);
    virtual ~MvdBrowserView();

    void setMovieCollection(MvdMovieCollection *c);

    int cacheMovieData(const MvdMovieData &md);
    void clearCachedMovieData(int id);

    void setControlsVisible(bool visible);
    bool controlsVisible() const;

    void showMovie(mvdid id);
    void showMovies(const QList<mvdid> &ids);

    void showCachedMovie(int id);

public slots:
    void clear();
    void blank();
    void setUrl(QString url);

    bool eventFilter(QObject *o, QEvent *e);

signals:
    void linkClicked(const QUrl &url);

protected:
    virtual void showContextMenu(QContextMenuEvent *e);

    QWebFrame *frame() const;
    QWebPage *page() const;
    QWebView *webView() const;

protected slots:
    void movieChanged(mvdid id);

private:
    class Private;
    Private *d;
};

#endif // MVD_BROWSERVIEW_H
