/**************************************************************************
** Filename: mainwindow.h
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

#ifndef MVD_MAINWINDOW_H
#define MVD_MAINWINDOW_H

#include "movieeditor.h"

#include "mvdcore/core.h"
#include "mvdcore/global.h"
#include "mvdcore/moviecollection.h"

#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QtGlobal>
#include <QtGui/QAbstractItemView>
#include <QtGui/QMainWindow>

class MvdBrowserView;
class MvdCollectionModel;
class MvdDockWidget;
class MvdFilterProxyModel;
class MvdFilterWidget;
class MvdMovieCollection;
class MvdMovieTreeView;
class MvdMovieViewListener;
class MvdPluginInterface;
class MvdPluginInterface;
class MvdRowSelectionModel;
class MvdSharedDataEditor;
class MvdSharedDataModel;
class MvdSmartView;

class QAction;
class QActionGroup;
class QEvent;
class QGridLayout;
class QHttp;
class QKeyEvent;
class QMenu;
class QMenuBar;
class QStackedWidget;
class QTemporaryFile;
class QTimer;
class QToolBar;
class QUrl;

namespace Movida {
extern void mainWindowMessageHandler(Movida::MessageType t, const QString &m);
}


class MvdMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MvdMainWindow(QWidget *parent = 0);
    virtual ~MvdMainWindow();
    void cleanUp();

    mvdid movieIndexToId(const QModelIndex &index) const;
    QModelIndex movieIdToIndex(mvdid id) const;

    virtual bool eventFilter(QObject *o, QEvent *e);

    QList<mvdid> selectedMovies() const;

    MvdFilterWidget *filterWidget() const;
    bool isQuickFilterVisible() const;

    QAbstractItemView* currentMovieView() const;

    void showTemporaryMessage(const QString &msg);
    void showPersistentMessage(const QString &msg);
    void hideMessages();

public slots:
    void setMoviePoster(quint32 movieId, const QUrl &url);

    bool newCollection(bool silent = false, bool *error = 0);
    bool closeCollection(bool silent = false, bool *error = 0)
    { return newCollection(silent, error); }
    bool loadCollection(const QString &file);
    void loadLastCollection();
    bool saveCollection(bool silent = false);

    void zoomIn();
    void zoomOut();

    void resetFilter();
    void filter(QString s);

    void lockToolBars(bool lock);

    void clearRecentFiles();

    void handleLinkClicked(const QUrl &url);

    void showLog();
    void showPreferences();

    void addMovie();
    void duplicateCurrentMovie();
    void editMovie(const QModelIndex &index);
    void editNextMovie();
    void editPreviousMovie();
    void editSelectedMovies();
    void massEditSelectedMovies();
    void removeMovie(const QModelIndex &index);
    void removeMovies(const QModelIndexList &list);
    void removeSelectedMovies();

    void showMovieContextMenu(const QModelIndex &index);

    void showCollectionMeta();
    void showFilterWidget();
    void toggleFilterWidget();
    void showSharedDataEditor();

signals:
    void movieSelectionChanged();

protected:
    virtual bool event(QEvent *e);
    virtual void closeEvent(QCloseEvent *e);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual QMenu *createPopupMenu();

private:
    friend void Movida::mainWindowMessageHandler(Movida::MessageType t, const QString &m);
    friend class MvdMovieViewListener;

    class Private;
    Private *d;
};

namespace Movida {
extern MvdMainWindow *MainWindow;
}

#endif // MVD_MAINWINDOW_H
