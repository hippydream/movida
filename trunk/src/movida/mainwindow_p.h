/**************************************************************************
** Filename: mainwindow_p.h
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Movida API.  It exists for the convenience
// of Movida.  This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//

#ifndef MVD_MAINWINDOW_P_H
#define MVD_MAINWINDOW_P_H

#include "mainwindow.h"

#include "mvdcore/core.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtGui/QSyntaxHighlighter>

class MvdBrowserView;
class MvdCollectionModel;
class MvdDockWidget;
class MvdFilterProxyModel;
class MvdFilterWidget;
class MvdInfoPanel;
class MvdMainWindow;
class MvdMovieCollection;
class MvdMovieEditor;
class MvdMovieTreeView;
class MvdPluginInterface;
class MvdRowSelectionModel;
class MvdSharedDataEditor;
class MvdSharedDataModel;
class MvdSmartView;

class QAction;
class QHttp;
class QMenu;
class QRegExp;
class QStackedWidget;
class QTemporaryFile;
class QTextCharFormat;
class QTimer;
class QToolBar;

using namespace Movida;


class MvdMainWindow::Private : public QObject
{
    Q_OBJECT

public:
    Private(MvdMainWindow * p) :
        mMovieModel(0),
        mSelectionModel(0),
        mSharedDataModel(0),
        mMovieEditor(0),
        mHttp(0),
        mPendingRemoteRequests(),
        mFilterWidget(0),
        mHideFilterTimer(0),
        mFilterModel(0),
        mDraggingSharedData(false),
        mSavedFilterMessage(0),
        mMainLayout(0),
        mA_FileExit(0),
        mA_FileNew(0),
        mA_FileOpen(0),
        mA_FileOpenLast(0),
        mA_FileImport(0),
        mA_FileExport(0),
        mA_FileRecent(0),
        mA_FileSave(0),
        mA_FileSaveAs(0),
        mA_ViewModeTree(0),
        mA_ViewModeSmart(0),
        mA_ViewModeZoom(0),
        mA_ViewModeZoomIn(0),
        mA_ViewModeZoomOut(0),
        mAG_ViewMode(0),
        mA_ViewDetails(0),
        mA_ViewSort(0),
        mAG_ViewSort(0),
        mA_ViewSortDescending(0),
        mA_CollAddMovie(0),
        mA_CollRemMovie(0),
        mA_CollEdtMovie(0),
        mA_CollDupMovie(0),
        mA_CollMeta(0),
        mA_ToolSdEditor(0),
        mA_ToolPref(0),
        mA_ToolLog(0),
        mA_HelpAbout(0),
        mA_HelpContents(0),
        mA_HelpIndex(0),
        mA_LockToolBars(0),
        mMN_File(0),
        mMN_View(0),
        mMN_Collection(0),
        mMN_Plugins(0),
        mMN_Tools(0),
        mMN_Help(0),
        mMN_FileMRU(0),
        mMN_FileImport(0),
        mMN_FileExport(0),
        mMN_ViewSort(0),
        mTB_MainToolBar(0),
        mInfoPanel(0),
        mInfoPanelClosedByUser(0),
        mMainViewStack(0),
        mSmartView(0),
        mTreeView(0),
        mDetailsView(0),
        mSharedDataEditor(0),
        mDetailsDock(0),
        mSharedDataDock(0),
        q(p)
    { }

    void setupUi();

    QAction *createAction();
    void createActions();
    void createMenus();
    void createToolBars();
    void initAction(QAction *action, const QString &text, const QString &shortInfo,
    const QString &longInfo, const QString &shortcut) const;
    void retranslateUi();
    void setupConnections();

    void loadPlugins();

    void updateActionTexts(int);
    void updateBrowserView();
    void updateViewSortMenu();

    void updateCaption();

    mvdid movieIndexToId(const QModelIndex &index) const;
    QModelIndex movieIdToIndex(mvdid id) const;

    void registerMessageHandler()
    { Movida::registerMessageHandler(mainWindowMessageHandler); }

    void cleanUp();

    bool shouldShowQuickFilter() const;
    bool isQuickFilterVisible() const;

    // Shared Data panel D&D
    void sdeDragStarted();

    void dispatchMessage(Movida::MessageType t, const QString &m);

    bool closeCollection(bool silent = false, bool *error = 0);

    void setMoviePoster(quint32 movieId, const QUrl &url);

public slots:
    void movieViewSelectionChanged();
    void currentViewChanged();

    void collectionMetaDataChanged(int t);
    void collectionModified();
    void movieAdded(mvdid id);
    void movieChanged(mvdid id);

    void collectionCreated(MvdMovieCollection* c);

    void pluginsUnloaded();
    void pluginActionTriggered();

    // Shared Data panel D&D
    void sdeDragEnded();

    void updateFileMenu();
    void updatePluginsMenu();

    void filter();
    void filter(QString s);

    void openRecentFile(QAction *a);
    bool collectionLoaderCallback(int state, const QVariant &data);
    bool loadCollectionDlg();
    bool saveCollectionDlg(bool silent = false);

    void addRecentFile(const QString &file);

    void showMovieContextMenu(const QModelIndex &index);

    void movieViewToggled(QAction *);
    void sortActionTriggered(QAction *a = 0);
    void treeViewSorted(int logicalIndex);

    void infoPanelClosedByUser()
    { mInfoPanelClosedByUser = true; }

    void httpRequestFinished(int id, bool error);

    void escape();

protected:
    void timerEvent(QTimerEvent *e);

public:
    // The current movie collection
    MvdCollectionModel *mMovieModel;
    MvdRowSelectionModel *mSelectionModel;
    MvdSharedDataModel *mSharedDataModel;

    QSharedPointer<MvdMovieEditor> mMovieEditor;
    QSharedPointer<MvdMovieEditor> mMovieMassEditor; //! \todo Use mMovieMassEditor

    // For various HTTP requests.
    struct RemoteRequest {
        enum { Invalid = 0, MoviePoster };

        RemoteRequest() :
            requestType(Invalid),
            requestId(-1),
            target(MvdNull),
            tempFile(0) { }

        QUrl url;
        quint16 requestType;
        int requestId;
        QVariant data;
        mvdid target;
        QTemporaryFile *tempFile;
    };

    //! Consider using network manager
    QHttp *mHttp;
    QList<RemoteRequest> mPendingRemoteRequests;

    // Filter bar
    MvdFilterWidget *mFilterWidget;
    QTimer *mHideFilterTimer;
    MvdFilterProxyModel *mFilterModel;

    // D&D
    bool mDraggingSharedData;
    int mSavedFilterMessage;


    //////////////////////////////////////////////////////////////////////////
    // GUI
    //////////////////////////////////////////////////////////////////////////

    QGridLayout *mMainLayout;

    // Actions
    QAction *mA_FileExit;
    QAction *mA_FileNew;
    QAction *mA_FileOpen;
    QAction *mA_FileOpenLast;
    QAction *mA_FileImport;
    QAction *mA_FileExport;
    QAction *mA_FileRecent;
    QAction *mA_FileSave;
    QAction *mA_FileSaveAs;

    QAction *mA_ViewModeTree;
    QAction *mA_ViewModeSmart;
    QAction *mA_ViewModeZoom;
    QAction *mA_ViewModeZoomIn;
    QAction *mA_ViewModeZoomOut;
    QActionGroup *mAG_ViewMode;
    QAction *mA_ViewDetails;
    QAction *mA_ViewSort;
    QActionGroup *mAG_ViewSort;
    QAction *mA_ViewSortDescending;

    QAction *mA_CollAddMovie;
    QAction *mA_CollRemMovie;
    QAction *mA_CollEdtMovie;
    QAction *mA_CollDupMovie;
    QAction *mA_CollMeta;

    QAction *mA_ToolSdEditor;
    QAction *mA_ToolPref;
    QAction *mA_ToolLog;

    QAction *mA_HelpAbout;
    QAction *mA_HelpContents;
    QAction *mA_HelpIndex;

    QAction *mA_LockToolBars;

    // Top level menus
    QMenu *mMN_File;
    QMenu *mMN_View;
    QMenu *mMN_Collection;
    QMenu *mMN_Plugins;
    QMenu *mMN_Tools;
    QMenu *mMN_Help;

    // Sub menus
    QMenu *mMN_FileMRU;
    QMenu *mMN_FileImport;
    QMenu *mMN_FileExport;
    QMenu *mMN_ViewSort;

    // Tool bars
    QToolBar *mTB_MainToolBar;

    // Info panel
    MvdInfoPanel *mInfoPanel;
    bool mInfoPanelClosedByUser;

    // Views
    QStackedWidget *mMainViewStack;
    MvdSmartView *mSmartView;
    MvdMovieTreeView *mTreeView;
    MvdBrowserView *mDetailsView;
    MvdSharedDataEditor *mSharedDataEditor;

    // Dock windows
    MvdDockWidget *mDetailsDock;
    MvdDockWidget *mSharedDataDock;

private slots:
#ifdef _MVD_DEBUG
    void openTempDir();
    void showPersistentMessage();
    void showTemporaryMessage();
#endif

private:
    MvdMainWindow *q;
};

/////////////////////////////////////////////////////////////

class MvdLogSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    MvdLogSyntaxHighlighter(QTextDocument *parent = 0);

protected:
    virtual void highlightBlock(const QString &text);

private:
    struct HighlightingRule
    {
     QRegExp pattern;
     QTextCharFormat format;
    };

    static QVector<HighlightingRule> mHighlightingRules;
    static bool mRulesInitialized;
};

#endif // MVD_MAINWINDOW_P_H
