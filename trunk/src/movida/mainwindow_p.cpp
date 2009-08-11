/**************************************************************************
** Filename: mainwindow_p.cpp
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

#include "mainwindow_p.h"

#include "collectionmodel.h"
#include "dockwidget.h"
#include "filterproxymodel.h"
#include "filterwidget.h"
#include "infopanel.h"
#include "movietreeview.h"
#include "movietreeviewdelegate.h"
#include "movieviewlistener.h"
#include "rowselectionmodel.h"
#include "shareddataeditor.h"
#include "smartview.h"

#include "mvdcore/collectionloader.h"
#include "mvdcore/collectionsaver.h"
#include "mvdcore/logger.h"
#include "mvdcore/pathresolver.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"

#include "mvdshared/browserview.h"

#include <QtCore/QDir>
#include <QtCore/QLibrary>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtGui/QFileDialog>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QShortcut>
#include <QtGui/QStackedWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtNetwork/QHttp>

using namespace Movida;

namespace {
struct PluginAction {
    QString name;
    int type;
};
}
Q_DECLARE_METATYPE(::PluginAction);


void MvdMainWindow::Private::setupUi()
{
#ifdef Q_OS_MAC
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    QFrame *container = new QFrame;
    container->setFrameShape(QFrame::StyledPanel);
    container->setFrameShadow(QFrame::Raised);
    q->setCentralWidget(container);

    QGridLayout *layout = new QGridLayout(container);
    layout->setMargin(0);
    layout->setSpacing(0);

    mInfoPanel = new MvdInfoPanel;
    mInfoPanel->closeImmediately(); // by-pass automatic delayed hiding
    layout->addWidget(mInfoPanel, 0, 0);

    mMainViewStack = new QStackedWidget;
    mMainViewStack->setFrameShadow(QFrame::Raised);
    layout->addWidget(mMainViewStack, 1, 0);

    q->statusBar()->setSizeGripEnabled(false);

    // Share filter proxy model
    mFilterModel = new MvdFilterProxyModel(q);
    mFilterModel->setSourceModel(mMovieModel);
    mFilterModel->setDynamicSortFilter(true);
    mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    mSmartView = new MvdSmartView(q);
    mSmartView->setObjectName("movie-smart-view");
    mSmartView->setModel(mFilterModel);

    mTreeView = new MvdMovieTreeView(q);
    mTreeView->setObjectName("movie-tree-view");
    mTreeView->setModel(mFilterModel);
    mTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTreeView->setDragEnabled(true);

    MvdMovieTreeViewDelegate *mtvd = new MvdMovieTreeViewDelegate(q);
    mTreeView->setItemDelegateForColumn(int(Movida::RatingAttribute), mtvd);
    mTreeView->setItemDelegateForColumn(int(Movida::SeenAttribute), mtvd);
    mTreeView->setItemDelegateForColumn(int(Movida::LoanedAttribute), mtvd);
    mTreeView->setItemDelegateForColumn(int(Movida::SpecialAttribute), mtvd);

    // Share selection model
    mSelectionModel = new MvdRowSelectionModel(mFilterModel);
    mTreeView->setSelectionModel(mSelectionModel);
    mSmartView->setSelectionModel(mSelectionModel);

    // Install event listener
    MvdMovieViewListener *listener = new MvdMovieViewListener(q);
    listener->registerView(mTreeView);
    listener->registerView(mSmartView);

    mMainViewStack->addWidget(mTreeView);
    mMainViewStack->addWidget(mSmartView);

    mDetailsDock = new MvdDockWidget(MvdMainWindow::tr("Details view"), q);
    mDetailsDock->setObjectName("details-dock");
    q->addDockWidget(Qt::RightDockWidgetArea, mDetailsDock);

    mDetailsView = new MvdBrowserView;
    mDetailsView->setControlsVisible(false);
    mDetailsDock->setWidget(mDetailsView);

    mSharedDataDock = new MvdDockWidget(MvdMainWindow::tr("Shared data"), q);
    mSharedDataDock->setObjectName("shared-data-dock");
    q->addDockWidget(Qt::RightDockWidgetArea, mSharedDataDock);

    mSharedDataEditor = new MvdSharedDataEditor;
    mSharedDataEditor->setModel(mSharedDataModel);
    mSharedDataDock->setWidget(mSharedDataEditor);

    mFilterWidget = new MvdFilterWidget;
    mFilterWidget->setVisible(false);
    mFilterWidget->editor()->installEventFilter(q);
    layout->addWidget(mFilterWidget, 2, 0);

    mHideFilterTimer = new QTimer(q);
    mHideFilterTimer->setInterval(5000);
    mHideFilterTimer->setSingleShot(true);

    createActions();
    createMenus();
    createToolBars();
    q->lockToolBars(true);

    // Intercept drag & drop. Need to install on viewport or D&D events will be blocked by the view.
    mSmartView->setAcceptDrops(true);
    mSmartView->viewport()->installEventFilter(q);
    mTreeView->setAcceptDrops(true);
    mTreeView->viewport()->installEventFilter(q);

    retranslateUi();
}

QAction *MvdMainWindow::Private::createAction()
{
    QAction *action = new QAction(q);

    return action;
}

//! \todo Consider loading actions from file
void MvdMainWindow::Private::createActions()
{
    // File menu
    mA_FileNew = createAction();
    mA_FileNew->setIcon(QIcon(":/images/document-new.svgz"));

    mA_FileOpen = createAction();
    mA_FileOpen->setIcon(QIcon(":/images/document-open.svgz"));

    mA_FileOpenLast = createAction();
    mA_FileOpenLast->setIcon(QIcon(":/images/document-reopen.svgz"));

    mA_FileImport = createAction();
    mA_FileExport = createAction();

    mA_FileRecent = createAction();

    mA_FileSave = createAction();
    mA_FileSave->setIcon(QIcon(":/images/document-save.svgz"));

    mA_FileSaveAs = createAction();
    mA_FileSaveAs->setIcon(QIcon(":/images/document-save-as.svgz"));

    mA_FileExit = createAction();
    mA_FileExit->setIcon(QIcon(":/images/application-exit.svgz"));


    // View menu
    mAG_ViewMode = new QActionGroup(q);

    mA_ViewModeSmart = createAction();
    mA_ViewModeSmart->setIcon(QIcon(":/images/fileview-detailed.svgz"));
    mA_ViewModeSmart->setCheckable(true);
    mAG_ViewMode->addAction(mA_ViewModeSmart);

    mA_ViewModeTree = createAction();
    mA_ViewModeTree->setIcon(QIcon(":/images/fileview-text.svgz"));
    mA_ViewModeTree->setCheckable(true);
    mA_ViewModeTree->setChecked(true);
    mAG_ViewMode->addAction(mA_ViewModeTree);

    mA_ViewModeZoom = createAction();

    mA_ViewModeZoomIn = createAction();
    mA_ViewModeZoomIn->setIcon(QIcon(":/images/zoom-in.svgz"));

    mA_ViewModeZoomOut = createAction();
    mA_ViewModeZoomOut->setIcon(QIcon(":/images/zoom-out.svgz"));


    // Bridge QAction to a movida specific action
    mA_ViewDetails = createAction();
    mA_ViewDetails->setIcon(QIcon(":/images/fileview-split.svgz"));
    mA_ViewDetails->setCheckable(true);
    mA_ViewDetails->setChecked(true);
    connect(mA_ViewDetails, SIGNAL(triggered(bool)), mDetailsDock, SLOT(setVisible(bool)));
    connect(mDetailsDock, SIGNAL(visibilityChanged(bool)), mA_ViewDetails, SLOT(setChecked(bool)));

    mAG_ViewSort = new QActionGroup(q);
    connect(mAG_ViewSort, SIGNAL(triggered(QAction *)), this, SLOT(sortActionTriggered(QAction *)));

    mA_ViewSort = createAction();

    mA_ViewSortDescending = createAction();
    mA_ViewSortDescending->setCheckable(true);
    connect(mA_ViewSortDescending, SIGNAL(triggered()), this, SLOT(sortActionTriggered()));


    // Collection menu
    mA_CollAddMovie = createAction();
    mA_CollAddMovie->setIcon(QIcon(":/images/edit-add.svgz"));
    mA_CollAddMovie->setShortcut(Qt::CTRL | Qt::Key_A);

    mA_CollRemMovie = createAction();
    mA_CollRemMovie->setIcon(QIcon(":/images/edit-delete.svgz"));

    mA_CollEdtMovie = createAction();
    mA_CollEdtMovie->setIcon(QIcon(":/images/edit.svgz"));

    mA_CollMedMovie = createAction();
    mA_CollMedMovie->setIcon(QIcon(":/images/mass-edit.svgz"));

    mA_CollDupMovie = createAction();
    mA_CollDupMovie->setIcon(QIcon(":/images/edit-copy.svgz"));

    mA_CollMeta = createAction();
    mA_CollMeta->setIcon(QIcon(":/images/document-properties.svgz"));


    // Tools menu
    mA_ToolSdEditor = createAction();
    mA_ToolSdEditor->setIcon(QIcon(":/images/identity.svgz"));

    mA_ToolPref = createAction();
    mA_ToolPref->setIcon(QIcon(":/images/configure.svgz"));

    mA_ToolLog = createAction();
    mA_ToolLog->setIcon(QIcon(":/images/list.svgz"));

    // Help menu
    mA_HelpContents = createAction();
    mA_HelpContents->setIcon(QIcon(":/images/help-contents.svgz"));

    mA_HelpIndex = createAction();

    //! \todo SVG application logo
    mA_HelpAbout = createAction();
    //mA_HelpAbout->setIcon(QIcon(":/images/32x32/logo"));

    // Misc actions
    mA_LockToolBars = createAction();
    mA_LockToolBars->setIcon(QIcon(":/images/lock-toolbars.svgz"));
    mA_LockToolBars->setCheckable(true);
    mA_LockToolBars->setChecked(true);
}

void MvdMainWindow::Private::createMenus()
{
    QMenuBar *menuBar = q->menuBar();

    mMN_File = menuBar->addMenu(MvdMainWindow::tr("&File"));
    mMN_View = menuBar->addMenu(MvdMainWindow::tr("&View"));
    mMN_Collection = menuBar->addMenu(MvdMainWindow::tr("&Collection"));
    mMN_Plugins = menuBar->addMenu(MvdMainWindow::tr("&Plugins"));
    mMN_Tools = menuBar->addMenu(MvdMainWindow::tr("&Tools"));
    mMN_Help = menuBar->addMenu(MvdMainWindow::tr("&Help"));

    mMN_FileImport = new QMenu(MvdMainWindow::tr("&Import"), q);
    mA_FileImport->setMenu(mMN_FileImport);

    mMN_FileExport = new QMenu(MvdMainWindow::tr("&Export"), q);
    mA_FileExport->setMenu(mMN_FileExport);

    mMN_FileMRU = new QMenu(MvdMainWindow::tr("&Recent"), q);
    mA_FileRecent->setMenu(mMN_FileMRU);

    mMN_ViewSort = new QMenu(MvdMainWindow::tr("&Sort by"), q);
    mA_ViewSort->setMenu(mMN_ViewSort);

    QMenu *zoomMenu = new QMenu(MvdMainWindow::tr("&Zoom level"), q);
    zoomMenu->addAction(mA_ViewModeZoomOut);
    zoomMenu->addAction(mA_ViewModeZoomIn);
    mA_ViewModeZoom->setMenu(zoomMenu);

    // Populate menus
    mMN_File->addAction(mA_FileNew);
    mMN_File->addSeparator();
    mMN_File->addAction(mA_FileOpen);
    mMN_File->addAction(mA_FileOpenLast);
    mMN_File->addAction(mA_FileRecent);
    mMN_File->addAction(mA_FileImport);
    mMN_File->addAction(mA_FileExport);
    mMN_File->addSeparator();
    mMN_File->addAction(mA_FileSave);
    mMN_File->addAction(mA_FileSaveAs);
    mMN_File->addSeparator();
    mMN_File->addAction(mA_FileExit);

    mMN_View->addAction(mA_ViewModeSmart);
    mMN_View->addAction(mA_ViewModeTree);
    mMN_View->addAction(mA_ViewModeZoom);
    mMN_View->addSeparator();
    mMN_View->addAction(mA_ViewDetails);
    mMN_View->addSeparator();
    mMN_View->addAction(mA_ViewSort);

    mMN_Collection->addAction(mA_CollAddMovie);
    mMN_Collection->addAction(mA_CollRemMovie);
    mMN_Collection->addAction(mA_CollEdtMovie);
    mMN_Collection->addAction(mA_CollMedMovie);
    mMN_Collection->addAction(mA_CollDupMovie);
    mMN_Collection->addSeparator();
    mMN_Collection->addAction(mA_CollMeta);

    mMN_Tools->addAction(mA_ToolSdEditor);
    mMN_Tools->addAction(mA_ToolPref);
    mMN_Tools->addAction(mA_ToolLog);

    mMN_Help->addAction(mA_HelpAbout);
    mMN_Help->addAction(mA_HelpContents);
    mMN_Help->addAction(mA_HelpIndex);
}

void MvdMainWindow::Private::createToolBars()
{
    mTB_MainToolBar = q->addToolBar(MvdMainWindow::tr("Main tools"));
    mTB_MainToolBar->setObjectName("main-toolbar");

    // File
    mTB_MainToolBar->addAction(mA_FileNew);
    mTB_MainToolBar->addAction(mA_FileOpen);
    mTB_MainToolBar->addSeparator();
    mTB_MainToolBar->addAction(mA_FileSave);
    mTB_MainToolBar->addAction(mA_FileSaveAs);

    mTB_MainToolBar->addSeparator();

    // Collection
    mTB_MainToolBar->addAction(mA_CollAddMovie);
    mTB_MainToolBar->addAction(mA_CollRemMovie);
    mTB_MainToolBar->addAction(mA_CollEdtMovie);

    mTB_MainToolBar->addSeparator();

    // Tools
    mTB_MainToolBar->addAction(mA_ToolPref);
    mTB_MainToolBar->addAction(mA_ToolSdEditor);

    mTB_MainToolBar->addSeparator();

    // Help
    mTB_MainToolBar->addAction(mA_HelpContents);

    mTB_MainToolBar->addSeparator();

    // Exit
    mTB_MainToolBar->addAction(mA_FileExit);
}

void MvdMainWindow::Private::initAction(QAction *action, const QString &text,
    const QString &shortInfo, const QString &longInfo, const QString &shortcut) const
{
    Q_ASSERT(action);

    action->setText(text);
    action->setToolTip(shortInfo);
    action->setStatusTip(shortInfo);
    action->setWhatsThis(longInfo);
    action->setShortcut(shortcut);
}

void MvdMainWindow::Private::retranslateUi()
{
    mDetailsDock->setWindowTitle(MvdMainWindow::tr("Details view"));

    QString text = MvdMainWindow::tr("&New Collection");
    QString shortInfo = MvdMainWindow::tr("Create a new movie collection");
    QString longInfo = shortInfo;
    QString shortcut = MvdMainWindow::tr("Ctrl+N");
    initAction(mA_FileNew, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Open Collection...");
    shortInfo = MvdMainWindow::tr("Open an existing movie collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+O");
    initAction(mA_FileOpen, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("Open &Last Collection");
    shortInfo = MvdMainWindow::tr("Open the last used movie collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+L");
    initAction(mA_FileOpenLast, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Import");
    shortInfo = MvdMainWindow::tr("Import movies from other applications or sources");
    longInfo = MvdMainWindow::tr("Import movies from other applications and file formats or from external sources like the Internet.");
    shortcut.clear();
    initAction(mA_FileImport, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Export");
    shortInfo = MvdMainWindow::tr("Export movies to other formats");
    longInfo = MvdMainWindow::tr("Export movies to other formats supported by other applications.");
    shortcut.clear();
    initAction(mA_FileExport, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("R&ecent Files");
    shortInfo = MvdMainWindow::tr("Re-open a recently used collection");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_FileRecent, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Save Collection");
    shortInfo = MvdMainWindow::tr("Save this movie collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+S");
    initAction(mA_FileSave, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("Save Collection &As...");
    shortInfo = MvdMainWindow::tr("Select a filename to save this collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+Shift+S");
    initAction(mA_FileSaveAs, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("E&xit");
    shortInfo = MvdMainWindow::tr("Exit Movida");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_FileExit, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Smart View");
    shortInfo = MvdMainWindow::tr("Show the movies using tiles");
    longInfo = MvdMainWindow::tr("Show the movies using tiles that contain the movie poster and most relevant information");
    shortcut = MvdMainWindow::tr("CTRL+1");
    initAction(mA_ViewModeSmart, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&List View");
    shortInfo = MvdMainWindow::tr("Show the movies as a detailed list");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("CTRL+2");
    initAction(mA_ViewModeTree, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Zoom level");
    shortInfo = MvdMainWindow::tr("Enlarge or reduce the smart view tiles");
    longInfo = MvdMainWindow::tr("Enlarge or reduce the smart view tiles to show more or less information");
    shortcut.clear();
    initAction(mA_ViewModeZoom, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Smaller tiles");
    shortInfo = MvdMainWindow::tr("Reduce the smart view tiles");
    longInfo = MvdMainWindow::tr("Reduce the smart view tiles to show less information");
    shortcut = MvdMainWindow::tr("CTRL+-");
    initAction(mA_ViewModeZoomOut, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Larger tiles");
    shortInfo = MvdMainWindow::tr("Enlarge the smart view tiles");
    longInfo = MvdMainWindow::tr("Enlarge the smart view tiles to show more information");
    shortcut = MvdMainWindow::tr("CTRL++");
    initAction(mA_ViewModeZoomIn, text, shortInfo, longInfo, shortcut);

    text = mDetailsDock->windowTitle();
    shortInfo = text;
    longInfo = text;
    shortcut.clear();
    initAction(mA_ViewDetails, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Sort by");
    shortInfo = MvdMainWindow::tr("Select the attribute used to sort the movies");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_ViewSort, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Descending");
    shortInfo = MvdMainWindow::tr("Sort movies descending");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_ViewSortDescending, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Add a new movie");
    shortInfo = MvdMainWindow::tr("Add a new movie to the collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+M");
    initAction(mA_CollAddMovie, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Remove selected movie");
    shortInfo = MvdMainWindow::tr("Removes the selected movie from the collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Delete");
    initAction(mA_CollRemMovie, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Edit selected movie");
    shortInfo = MvdMainWindow::tr("Open a dialog to edit the selected movie");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+E");
    initAction(mA_CollEdtMovie, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Mass edit selected movies");
    shortInfo = MvdMainWindow::tr("Open a dialog to assign common properties to the selected movies");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+Shift+E");
    initAction(mA_CollMedMovie, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Duplicate selected movie");
    shortInfo = MvdMainWindow::tr("Duplicate the selected movie");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_CollDupMovie, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("A&bout this collection...");
    shortInfo = MvdMainWindow::tr("Show or edit basic information about this collection");
    longInfo = MvdMainWindow::tr("Show or edit basic information about this collection, like name, owner, contact info etc.");
    shortcut.clear();
    initAction(mA_CollMeta, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Shared data editor");
    shortInfo = MvdMainWindow::tr("View and edit shared data");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_ToolSdEditor, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Preferences");
    shortInfo = MvdMainWindow::tr("Configure Movida");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_ToolPref, text, shortInfo, longInfo, shortcut);


    text = MvdMainWindow::tr("Show &Log");
    shortInfo = MvdMainWindow::tr("Shows the application log file");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_ToolLog, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Contents");
    shortInfo = MvdMainWindow::tr("Open the Movida user guide");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_HelpContents, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Index");
    shortInfo = MvdMainWindow::tr("Open the Movida user guide index");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_HelpIndex, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&About Movida");
    shortInfo = MvdMainWindow::tr("Show some information about Movida");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_HelpAbout, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Lock toolbars");
    shortInfo = MvdMainWindow::tr("Lock toolbars");
    longInfo = shortInfo;
    shortcut.clear();
    initAction(mA_LockToolBars, text, shortInfo, longInfo, shortcut);

    //! \todo re-translate menus and toolbars

    updateCaption();
}

/*!
    Signal/slot connections.
*/
void MvdMainWindow::Private::setupConnections()
{
    connect(mA_FileExit, SIGNAL(triggered()), q, SLOT(close()));
    connect(mA_FileNew, SIGNAL(triggered()), q, SLOT(newCollection()));
    connect(mA_FileOpen, SIGNAL(triggered()), this, SLOT(loadCollectionDlg()));
    connect(mA_FileOpenLast, SIGNAL(triggered()), q, SLOT(loadLastCollection()));
    connect(mA_FileSaveAs, SIGNAL(triggered()), this, SLOT(saveCollectionDlg()));
    connect(mA_FileSave, SIGNAL(triggered()), q, SLOT(saveCollection()));

    connect(mA_ToolPref, SIGNAL(triggered()), q, SLOT(showPreferences()));
    connect(mA_ToolLog, SIGNAL(triggered()), q, SLOT(showLog()));
    connect(mA_ToolSdEditor, SIGNAL(triggered()), q, SLOT(showSharedDataEditor()));

    connect(mAG_ViewMode, SIGNAL(triggered(QAction *)), this, SLOT(movieViewToggled(QAction *)));
    connect(mA_ViewModeZoomIn, SIGNAL(triggered()), q, SLOT(zoomIn()));
    connect(mA_ViewModeZoomOut, SIGNAL(triggered()), q, SLOT(zoomOut()));

    connect(mMN_FileMRU, SIGNAL(triggered(QAction *)), this, SLOT(openRecentFile(QAction *)));
    connect(mMN_File, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()));
    connect(mMN_Plugins, SIGNAL(aboutToShow()), this, SLOT(updatePluginsMenu()));

    connect(mA_CollAddMovie, SIGNAL(triggered()), q, SLOT(addMovie()));
    connect(mA_CollRemMovie, SIGNAL(triggered()), q, SLOT(removeSelectedMovies()));
    connect(mA_CollEdtMovie, SIGNAL(triggered()), q, SLOT(editSelectedMovies()));
    connect(mA_CollMedMovie, SIGNAL(triggered()), q, SLOT(massEditSelectedMovies()));
    connect(mA_CollDupMovie, SIGNAL(triggered()), q, SLOT(duplicateCurrentMovie()));
    connect(mA_CollMeta, SIGNAL(triggered()), q, SLOT(showCollectionMeta()));

    connect(mA_LockToolBars, SIGNAL(toggled(bool)), q, SLOT(lockToolBars(bool)));

    connect(mTreeView, SIGNAL(doubleClicked(const QModelIndex &)), q, SLOT(editMovie(const QModelIndex &)));
    connect(mTreeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this, SLOT(movieViewSelectionChanged()));
    connect(mTreeView, SIGNAL(contextMenuRequested(const QModelIndex &, QContextMenuEvent::Reason)),
        q, SLOT(showMovieContextMenu(const QModelIndex &)));
    connect(mTreeView->header(), SIGNAL(sectionClicked(int)), this, SLOT(treeViewSorted(int)));

    connect(mSmartView, SIGNAL(doubleClicked(const QModelIndex &)), q, SLOT(editMovie(const QModelIndex &)));
    connect(mSmartView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this, SLOT(movieViewSelectionChanged()));
    connect(mSmartView, SIGNAL(contextMenuRequested(const QModelIndex &)),
        q, SLOT(showMovieContextMenu(const QModelIndex &)));

    connect(mMainViewStack, SIGNAL(currentChanged(int)), this, SLOT(currentViewChanged()));

    connect(mDetailsView, SIGNAL(linkClicked(QUrl)), q, SLOT(handleLinkClicked(QUrl)));

    connect(mFilterWidget, SIGNAL(hideRequest()), q, SLOT(resetFilter()));
    connect(mFilterWidget, SIGNAL(caseSensitivityChanged()), this, SLOT(filter()));
    connect(mFilterWidget->editor(), SIGNAL(textChanged(QString)), this, SLOT(filter(QString)));

    connect(mHideFilterTimer, SIGNAL(timeout()), mFilterWidget, SLOT(hide()));
    connect(mInfoPanel, SIGNAL(closedByUser()), this, SLOT(infoPanelClosedByUser()));

    // Application shortcuts
    connect(new QShortcut(Qt::CTRL + Qt::Key_F, q), SIGNAL(activated()), q, SLOT(showFilterWidget()));
}

void MvdMainWindow::Private::movieViewSelectionChanged()
{
    QModelIndexList list = mTreeView->selectedRows();

    mA_CollRemMovie->setEnabled(!list.isEmpty());
    mA_CollEdtMovie->setEnabled(list.size() == 1);
    mA_CollMedMovie->setEnabled(list.size() > 1);
    mA_CollDupMovie->setEnabled(list.size() == 1);

    updateBrowserView();
}

void MvdMainWindow::Private::updateBrowserView()
{
    QList<mvdid> ids = q->selectedMovies();
    mDetailsView->showMovies(ids);
}

//! Updates GUI elements after the current view has changed (e.g. tree -> smart view).
void MvdMainWindow::Private::currentViewChanged()
{
    updateViewSortMenu();
}

//! Populates the sort menu according to the current view.
void MvdMainWindow::Private::updateViewSortMenu()
{
    mMN_ViewSort->clear();
    foreach(QAction * a, mAG_ViewSort->actions())
    mAG_ViewSort->removeAction(a);

    QList<Movida::MovieAttribute> atts;
    QWidget *cw = mMainViewStack->currentWidget();

    if (cw == mSmartView) {
        atts = Movida::movieAttributes(Movida::SmartViewAttributeFilter);
    } else if (cw == mTreeView) {
        atts = Movida::movieAttributes();
    }

    Movida::MovieAttribute currentAttribute = mFilterModel->sortAttribute();

    for (int i = 0; i < atts.size(); ++i) {
        Movida::MovieAttribute a = atts.at(i);
        QAction *action = mAG_ViewSort->addAction(Movida::movieAttributeString(a));
        action->setCheckable(true);
        if (a == currentAttribute)
            action->setChecked(true);
        action->setData((int)a);
    }

    mMN_ViewSort->addActions(mAG_ViewSort->actions());
    mMN_ViewSort->addSeparator();
    mMN_ViewSort->addAction(mA_ViewSortDescending);
}

/*!
    \internal
    Creates a new collection. Sets the collection as the view's model and
    connects the proper signals and slots.
*/
void MvdMainWindow::Private::createNewCollection()
{
    if (mCollection) {
        delete mCollection;
    }

    mCollection = new MvdMovieCollection();
    MvdCore::pluginContext()->collection = mCollection;
    mMovieModel->setMovieCollection(mCollection);
    mSharedDataModel->setMovieCollection(mCollection);
    mDetailsView->setMovieCollection(mCollection);

    connect(mCollection, SIGNAL(modified()), this, SLOT(collectionModified()));
    connect(mCollection, SIGNAL(metaDataChanged(int, QString)), this, SLOT(collectionMetaDataChanged(int)));
    connect(mCollection, SIGNAL(movieChanged(mvdid)), this, SLOT(movieChanged(mvdid)));
}

//! Called when the collection is modified.
void MvdMainWindow::Private::collectionModified()
{
    bool isEmpty = mCollection->isEmpty();
    bool isNewCollection = mCollection->path().isEmpty();
    bool isModified = mCollection->isModified();

    q->setWindowModified(isModified);

    mA_FileNew->setDisabled(isNewCollection && isEmpty);
    mA_FileSave->setDisabled(!isModified || isNewCollection || isEmpty);
    mA_FileSaveAs->setDisabled(isEmpty);
}

//! Called when the collection meta data is modified.
void MvdMainWindow::Private::collectionMetaDataChanged(int _t)
{
    MvdMovieCollection::MetaDataType t = (MvdMovieCollection::MetaDataType)(_t);

    if (t == MvdMovieCollection::NameInfo)
        updateCaption();
}

//! Called when a movie in the collection is modified.
void MvdMainWindow::Private::movieChanged(mvdid id)
{
    mvdid current = movieIndexToId(mTreeView->selectedIndex());

    if (current == 0)
        return;
}

/*!
    Updates the main window title.
*/
void MvdMainWindow::Private::updateCaption()
{
    QString windowTitle;

    if (mCollection)
        windowTitle = mCollection->metaData(MvdMovieCollection::NameInfo);

    if (mCollection && !windowTitle.isEmpty()) {
        ;
    } else if (!mCollection || mCollection->fileName().isEmpty()) {
        windowTitle = MvdMainWindow::tr("New Collection");
    } else {
        if (!mCollection->path().isEmpty())
            windowTitle = mCollection->path();
        else windowTitle = mCollection->fileName();
    }

    windowTitle.append(QLatin1String("[*]"));

#ifndef Q_WS_MAC
    QString appName = QApplication::applicationName();
    if (!appName.isEmpty())
        windowTitle += QLatin1String(" ") + QChar(0x2014) + QLatin1String(" ") + appName;
#endif

    q->setWindowTitle(windowTitle);
    q->setWindowModified(mCollection && mCollection->isModified());
}

mvdid MvdMainWindow::Private::movieIndexToId(const QModelIndex &index) const
{
    bool ok;
    mvdid id = mFilterModel->data(index, Movida::IdRole).toUInt(&ok);

    return ok ? id : MvdNull;
}

QModelIndex MvdMainWindow::Private::movieIdToIndex(mvdid id) const
{
    QModelIndex index = mMovieModel->findMovie(id);

    return index.isValid() ? mFilterModel->mapFromSource(index) : QModelIndex();
}

//! \internal Loads plugins from all the available plugin dirs.
void MvdMainWindow::Private::loadPlugins()
{
    unloadPlugins();

    mMN_Plugins->setEnabled(true);

    QString path = paths().resourcesDir(Movida::UserScope).append("Plugins");
    loadPluginsFromDir(path);
    path = paths().resourcesDir(Movida::SystemScope).append("Plugins");
    if (!path.isEmpty())
        loadPluginsFromDir(path);

    if (mMN_Plugins->isEmpty())
        mMN_Plugins->setEnabled(false);
}

//! \internal Loads plugins from the given plugins dir.
void MvdMainWindow::Private::loadPluginsFromDir(const QString &path)
{
    if (!mCollection)
        createNewCollection();

    QDir pluginDir(path);

#if defined (Q_WS_WIN)
    QString ext = "*.dll";
    QString prefix = "mpi";
#elif defined (Q_WS_MAC)
    QString ext = "*.dylib";
    QString prefix = "libmpi";
#else
    QString ext = "*.so";
    QString prefix = "libmpi";
#endif

    QFileInfoList list = pluginDir.entryInfoList(QStringList() << ext);
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo &fi = list[i];
        QString name = fi.completeBaseName();

        if (!name.startsWith(prefix))
            continue;

        QLibrary myLib(fi.absoluteFilePath());
        if (!myLib.load()) {
            eLog() << QString("Failed to load %1 (reason: %2)")
                .arg(fi.absoluteFilePath()).arg(myLib.errorString());
            continue;
        }

        iLog() << "Checking plugin " << fi.absoluteFilePath();

        typedef MvdPluginInterface * (*PluginInterfaceF)(QObject *);
        PluginInterfaceF pluginInterfaceF = (PluginInterfaceF)myLib.resolve("pluginInterface");
        if (!pluginInterfaceF)
            continue;

        MvdPluginInterface *iface = pluginInterfaceF(q);
        if (!iface)
            continue;

        MvdPluginInterface::PluginInfo info = iface->info();
        if (info.uniqueId.trimmed().isEmpty()) {
            wLog() << "Discarding plugin with no unique ID.";
            continue;
        }

        info.uniqueId.replace(QLatin1Char('/'), QLatin1Char('.'));

        bool discard = false;
        foreach(MvdPluginInterface * plug, mPlugins)
        {
            QString s = plug->info().uniqueId.trimmed();

            if (s.replace(QLatin1Char('/'), QLatin1Char('.')) == info.uniqueId.trimmed()) {
                discard = true;
                break;
            }
        }
        if (discard) {
            wLog() << QString("Discarding plugin with duplicate ID '%1'.").arg(info.uniqueId);
            continue;
        }

        if (info.name.trimmed().isEmpty()) {
            wLog() << "Discarding plugin with no name.";
            continue;
        }

        iLog() << QString("Loading '%1' plugin.").arg(info.uniqueId);

        QString dataStorePath = paths().resourcesDir(Movida::UserScope).append("Plugins/").append(fi.completeBaseName());
        if (!QFile::exists(dataStorePath)) {
            QDir d;
            if (!d.mkpath(dataStorePath)) {
                eLog() << "Failed to create user data store for plugin: " << dataStorePath;
                continue;
            }
        }

        dataStorePath = MvdCore::toLocalFilePath(dataStorePath, true);
        iface->setDataStore(dataStorePath, Movida::UserScope);
        iLog() << QString("'%1' plugin user data store: ").arg(info.name).append(dataStorePath);

        // Create global data store
        dataStorePath = paths().resourcesDir(Movida::SystemScope);
        if (!dataStorePath.isEmpty()) {
            dataStorePath.append("Plugins/").append(fi.completeBaseName());
            bool ok = true;
            if (!QFile::exists(dataStorePath)) {
                QDir d;
                if (!d.mkpath(dataStorePath)) {
                    wLog() << "Failed to create global data store for plugin: " << dataStorePath;
                    ok = false;
                }
            } else ok = true;

            if (ok) {
                dataStorePath = MvdCore::toLocalFilePath(dataStorePath, true);
                iface->setDataStore(dataStorePath, Movida::SystemScope);
                iLog() << QString("'%1' plugin system data store: ").arg(info.name).append(dataStorePath);
            }
        }


        QList<MvdPluginInterface::PluginAction> actions;

        // Initialize plugin
        bool pluginOk = iface->init();
        if (pluginOk) {
            iLog() << QString("'%1' plugin initialized.").arg(info.name);
            actions = iface->actions();
        } else wLog() << QString("Failed to initialize '%1' plugin.").arg(info.name);

        //! \todo sort plugin names?
        QMenu *pluginMenu = mMN_Plugins->addMenu(info.name);
        if (actions.isEmpty()) {
            pluginMenu->setEnabled(false);
        }

        for (int j = 0; j < actions.size(); ++j) {
            const MvdPluginInterface::PluginAction &a = actions.at(j);
            QAction *qa = createAction();
            qa->setText(a.text);
            qa->setStatusTip(a.helpText);
            qa->setIcon(a.icon);

                ::PluginAction pa;
            pa.name = QString("%1/%2").arg(info.uniqueId.trimmed()).arg(a.name);
            pa.type = a.type;
            qa->setData(QVariant::fromValue<PluginAction>(pa));

            connect(qa, SIGNAL(triggered()), this, SLOT(pluginActionTriggered()));

            pluginMenu->addAction(qa);

            if (a.type & MvdPluginInterface::ImportAction) {
                mMN_FileImport->addAction(qa);
            }

            if (a.type & MvdPluginInterface::ExportAction) {
                mMN_FileExport->addAction(qa);
            }
        }

        mPlugins.append(iface);
    }
}

//! \internal Unloads plugins and frees used memory. Also clears the plugins menu.
void MvdMainWindow::Private::unloadPlugins()
{
    if (mPlugins.isEmpty())
        return;

    while (!mPlugins.isEmpty()) {
        MvdPluginInterface *p = mPlugins.takeFirst();
        iLog() << QLatin1String("Unloading plugin: ") << p->info().name;
        p->unload();
        delete p;
    }

    mMN_Plugins->clear();
}

void MvdMainWindow::Private::pluginActionTriggered()
{
    QAction *a = qobject_cast<QAction *>(sender());

    if (!a) return;

        ::PluginAction pa = a->data().value<PluginAction>();
    QStringList l = pa.name.split(QLatin1Char('/'));
    if (l.size() != 2) return;

    QString pluginName = l[0];
    QString actionName = l[1];

    MvdPluginInterface *iface = 0;
    foreach(MvdPluginInterface * i, mPlugins)
    {
        QString s = i->info().uniqueId.trimmed();

        if (s.replace(QLatin1Char('/'), QLatin1Char('.')) == pluginName) {
            iface = i;
            break;
        }
    }

    if (!iface) return;

    MvdPluginContext *context = MvdCore::pluginContext();
    Q_ASSERT(context);

    context->selectedMovies = q->selectedMovies();

    // Update plugin context
    iface->actionTriggered(actionName);

    // Consume known properties
    QHash<QString, QVariant>::Iterator it = context->properties.begin();
    while (it != context->properties.end()) {
        QStringList key = it.key().split(QLatin1Char('/'), QString::SkipEmptyParts);
        if (key.size() < 3) continue;
        if (key[0] != QLatin1String("movida")) continue;

        QString k = key[1];
        if (k == QLatin1String("movies")) {
            k = key[2];
            if (k == QLatin1String("filter")) {
                QString ids = it.value().toString();
                QString query = QString("@%1(%2)").arg(Movida::filterFunctionName(Movida::MovieIdFilter)).arg(ids);
                mFilterWidget->editor()->setText(query);
            }
        }

        ++it;
    }

    context->properties.clear();
}

void MvdMainWindow::Private::cleanUp()
{
    MvdSettings &p = Movida::settings();

    p.setValue("movida/appearance/main-window-state", q->saveState());
    p.setValue("movida/appearance/start-maximized", q->isMaximized());
    if (!q->isMaximized()) {
        p.setValue("movida/appearance/main-window-size", q->size());
        p.setValue("movida/appearance/main-window-pos", q->pos());
    }
    p.setValue("movida/quick-filter/case-sensitive", mFilterWidget->caseSensitivity() == Qt::CaseSensitive);
    if (mMainViewStack->currentWidget() == mTreeView)
        p.setValue("movida/view-mode", "tree");
    else p.setValue("movida/view-mode", "smart");
    if (mSmartView->itemSize() == MvdSmartView::LargeItemSize)
        p.setValue("movida/smart-view/item-size", "large");
    else if (mSmartView->itemSize() == MvdSmartView::SmallItemSize)
        p.setValue("movida/smart-view/item-size", "small");
    else p.setValue("movida/smart-view/item-size", "medium");

    unloadPlugins();

    paths().removeDirectoryTree(paths().tempDir());

    if (mCollection && mCollection->isModified()) {
        //! \todo Emergency save!
    }

    // This must be called last!!! Singletons should not be accessed from now on!
    MvdCore::storeStatus();
}

//! Returns false if the quick filter should not be shown after a key press (e.g. because focus is on the SD editor)
bool MvdMainWindow::Private::shouldShowQuickFilter() const
{
    if (!mCollection || mCollection->isEmpty())
        return false;

    // Shared data editor must not loose its focus
    QWidget *w = QApplication::focusWidget();
    while (w) {
        if (w == mSharedDataDock)
            return false;
        w = qobject_cast<QWidget *>(w->parent());
    }

    return true;
}

bool MvdMainWindow::Private::isQuickFilterVisible() const
{
    return mFilterWidget->isVisible();
}

/*! Called when a drag from the SD panel has started, shows the filter bar
    with an appropriate message.
*/
void MvdMainWindow::Private::sdeDragStarted()
{
    mDraggingSharedData = true;
    mFilterWidget->show();
    mSavedFilterMessage = (int)mFilterWidget->message();
    mFilterWidget->setMessage(MvdFilterWidget::DropInfo);
    mHideFilterTimer->stop();
}

//! Called when the drag from the SD panel has been completed or canceled.
void MvdMainWindow::Private::sdeDragEnded()
{
    mDraggingSharedData = false;
    mFilterWidget->setMessage((MvdFilterWidget::Message)mSavedFilterMessage);
    if (mFilterWidget->editor()->text().trimmed().isEmpty() && !mFilterWidget->editor()->hasFocus()) {
        q->resetFilter();
    }
}

/*!
    Checks and updates the status of the file menu.
    This method is called before the file menu is shown.
*/
void MvdMainWindow::Private::updateFileMenu()
{
    // RECENT FILES

    QStringList list = Movida::settings().value("movida/recent-files").toStringList();
    bool updateList = false;

    for (int i = 0; i < list.size(); ++i) {
        if (!QFile::exists(list.at(i))) {
            list.removeAt(i);
            updateList = true;
        }
    }

    if (updateList)
        Movida::settings().setValue("movida/recent-files", list);

    mA_FileOpenLast->setDisabled(list.isEmpty());
    mMN_FileMRU->setDisabled(list.isEmpty());
    mMN_FileMRU->clear();

    for (int i = 0; i < list.size(); ++i) {
        //! \todo elide long filenames in MRU menu
        QAction *a = mMN_FileMRU->addAction(QString("&%1 %2").arg(i).arg(list.at(i)));
        a->setData(QVariant(list.at(i)));
    }

    // IMPORT/EXPORT
    bool hasMovies = mCollection && !mCollection->isEmpty();
    mMN_FileImport->setDisabled(mMN_FileImport->isEmpty());
    mMN_FileExport->setDisabled(!hasMovies || mMN_FileExport->isEmpty());

    if (hasMovies && !mMN_FileExport->isEmpty()) {
        updatePluginsMenu();
    }
}

/*!
    Checks and updates the status of the Plugins menu.
    This method is called before the plugins menu is shown.
*/
void MvdMainWindow::Private::updatePluginsMenu()
{
    if (mMN_Plugins->isEmpty())
        return;

    bool hasMovies = mCollection && !mCollection->isEmpty();

    QList<QAction *> l = mMN_Plugins->actions();
    for (int i = 0; i < l.size(); ++i) {
        QMenu *m = l.at(i)->menu();
        if (!m) continue;
        QList<QAction *> ml = m->actions();
        for (int j = 0; j < ml.size(); ++j) {
            QAction *a = ml.at(j);
                ::PluginAction pa = a->data().value<PluginAction>();
            MvdPluginInterface::ActionTypes types = (MvdPluginInterface::ActionTypes)pa.type;
            if (types & MvdPluginInterface::ExportAction) {
                a->setEnabled(hasMovies);
            }
        }
    }
}

//! Reapplies the current filter to the movie view. Use this when some option changed (e.g. case sensitivity).
void MvdMainWindow::Private::filter()
{
    filter(mFilterWidget->editor()->text());
}

void MvdMainWindow::Private::filter(QString s)
{
    bool filterWasVisible = mFilterWidget->isVisible();

    mFilterWidget->setMessage(MvdFilterWidget::NoMessage);

    s = s.trimmed();
    mHideFilterTimer->stop();

    QPalette p = mFilterWidget->editor()->palette();
    p.setColor(QPalette::Active, QPalette::Base, Qt::white);

    // PERFORM FILTER
    bool nothingToFilter = true;
    bool hasText = !mFilterWidget->editor()->text().trimmed().isEmpty();
    Qt::CaseSensitivity cs = mFilterWidget->caseSensitivity();

    mFilterModel->setFilterCaseSensitivity(cs);
    bool syntaxError = !mFilterModel->setFilterAdvancedString(s);
    if (syntaxError) {
        mFilterWidget->setMessage(MvdFilterWidget::SyntaxErrorWarning);
    }

    nothingToFilter = mFilterModel->rowCount() == 0;

    if (nothingToFilter && hasText)
        p.setColor(QPalette::Active, QPalette::Base, QColor(255, 102, 102));

    if (!mFilterWidget->isVisible())
        mFilterWidget->show();

    if (!syntaxError)
        mFilterWidget->setMessage((nothingToFilter && hasText) ?
            MvdFilterWidget::NoResultsWarning : MvdFilterWidget::NoMessage);

    mFilterWidget->editor()->setPalette(p);
    if (!mFilterWidget->editor()->hasFocus() && !hasText)
        mHideFilterTimer->start();

    if (!filterWasVisible)
        mInfoPanelClosedByUser = false;

    if (!s.isEmpty() && !mInfoPanelClosedByUser) {
        mInfoPanel->setText(MvdMainWindow::tr("Movie filter is active. Clear or close the filter bar to show all movies."));
        mInfoPanel->show();
    } else mInfoPanel->hide();
}

void MvdMainWindow::Private::dispatchMessage(Movida::MessageType t, const QString &m)
{
    switch (t) {
        case Movida::InformationMessage:
            mInfoPanel->showTemporaryMessage(m); break;

        case Movida::WarningMessage:
            mInfoPanel->showTemporaryMessage(m); break;

        case Movida::ErrorMessage:
            mInfoPanel->showTemporaryMessage(m); break;

        default:
            ;
    }
}

void Movida::mainWindowMessageHandler(Movida::MessageType t, const QString &m)
{
    if (MainWindow)
        MainWindow->d->dispatchMessage(t, m);
}

/*!
    Re-opens a recently opened archive.
*/
void MvdMainWindow::Private::openRecentFile(QAction *a)
{
    if (a == 0)
        return;

    if (!closeCollection())
        return;

    QString file = a->data().toString();
    q->loadCollection(file);

    // Move file to the top of the MRU list
    QStringList list = Movida::settings().value("movida/recent-files").toStringList();
    int idx = list.indexOf(file);
    if (idx < 0)
        return;
    list.removeAt(idx);
    list.prepend(file);
    Movida::settings().setValue("movida/recent-files", list);
}

//! This slot is called by the collection loader in order to report the progress.
bool MvdMainWindow::Private::collectionLoaderCallback(int state, const QVariant &data)
{
    if (state == MvdCollectionLoader::CollectionInfo) {

        return true;

        MvdCollectionLoader::Info info = data.value<MvdCollectionLoader::Info>();
        QString msg = MvdMainWindow::tr("Collection %1 contains %2 movies. Continue?")
                          .arg(info.metadata[QString("name")])
                          .arg(info.expectedMovieCount);

        int res = QMessageBox::question(q, MVD_CAPTION, msg, QMessageBox::Yes, QMessageBox::No);

        return (res == QMessageBox::Yes);

    } else if (state == MvdCollectionLoader::ProgressInfo) {

        int percent = data.toInt();

    }

    return true;
}

/*!
    Opens a file selection dialog.
    Returns false if the collection has not been loaded
    (this is not necessarily an error).
*/
bool MvdMainWindow::Private::loadCollectionDlg()
{
    if (!closeCollection())
        return false;

    MvdSettings &p = Movida::settings();

    QString lastDir = p.value("movida/directories/collection").toString();

    QString file = QFileDialog::getOpenFileName(
        q, MVD_CAPTION,
        lastDir,
        MvdMainWindow::tr("Movida Movie Collection (*.mmc)")
        );

    if (file.isNull())
        return false;

    int sep = file.lastIndexOf("/");
    if (sep > 0) {
        if (p.value("movida/directories/remember").toBool())
            p.setValue("movida/directories/collection", file.left(sep));
    }

    addRecentFile(file);
    return q->loadCollection(file);
}

/*!
    Shows a file selection dialog and saves the collection.
    Returns false if the collection has not been saved
    (this is not necessarily an error).
*/
bool MvdMainWindow::Private::saveCollectionDlg()
{
    Q_ASSERT(mCollection);
    Q_ASSERT(!mCollection->isEmpty());

    QString lastDir = Movida::settings().value("movida/directories/collection").toString();

    QString filename = QFileDialog::getSaveFileName(q, MVD_CAPTION, lastDir, "*.mmc");
    if (filename.isEmpty())
        return false;

    if (!filename.endsWith(".mmc"))
        filename.append(".mmc");

    addRecentFile(filename);

    int sep = filename.lastIndexOf("/");
    if (sep > 0) {
        if (Movida::settings().value("movida/directories/remember").toBool())
            Movida::settings().setValue("movida/directories/collection", filename.left(sep));
    }

    MvdCollectionSaver saver(q);
    MvdCollectionSaver::StatusCode res = saver.save(mCollection, filename);

    if (res != MvdCollectionSaver::NoError) {
        QMessageBox::warning(q, MVD_CAPTION, MvdMainWindow::tr("Failed to save the collection."));
        return false;
    }

    mCollection->setModifiedStatus(false);
    mA_FileSave->setEnabled(false);
    updateCaption();
    return true;
}

/*!
    Closes the current collection and initializes a new empty collection.
    Returns false if the collection has not been closed.
*/
bool MvdMainWindow::Private::closeCollection()
{
    q->resetFilter();

    if (!mCollection)
        return true;

    int res = QMessageBox::Discard;

    bool isNewEmptyCollection = mCollection->path().isEmpty() && mCollection->isEmpty();

    if (mCollection->isModified() && !isNewEmptyCollection) {
        res = QMessageBox::question(q, MVD_CAPTION,
            MvdMainWindow::tr("The collection has been modified.\nDo you want to save the changes?"),
            QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel);
    }

    if (res == QMessageBox::Cancel)
        return false;

    if (res == QMessageBox::Save) {
        bool result;
        if (mCollection->path().isEmpty())
            result = saveCollectionDlg();
        else result = q->saveCollection();

        if (!result)
            return false;
    }

    mA_FileNew->setDisabled(true);
    mA_FileSave->setDisabled(true);
    mA_FileSaveAs->setDisabled(true);

    updateCaption();

    mCollection->clearPersistentData();
    createNewCollection();

    return true;
}

/*!
    Adds a new item to the MRU files menu.
*/
void MvdMainWindow::Private::addRecentFile(const QString &file)
{
    QFileInfo fi(file);
    QStringList list = Movida::settings().value("movida/recent-files").toStringList();

    // avoid duplicate entries
    for (int i = 0; i < list.size(); ++i) {
        if (fi == QFileInfo(list.at(i)))
            return;
    }

    list.prepend(file);

    int max = Movida::settings().value("movida/maximum-recent-files").toInt();
    while (list.size() > max)
        list.removeLast();

    Movida::settings().setValue("movida/recent-files", list);

    mA_FileOpenLast->setDisabled(list.isEmpty());
}

void MvdMainWindow::Private::showMovieContextMenu(const QModelIndex &index)
{
    //! \todo Refactor menu in order to use application wide actions

    QWidget *senderWidget = mMainViewStack->currentWidget();

    Q_ASSERT(senderWidget);

    QMenu menu;
    QAction *addNew = 0;
    QAction *editCurrent = 0;
    QAction *editSelected = 0;
    QAction *deleteCurrent = 0;
    QAction *deleteSelected = 0;

    bool currentIsSelected = false;
    bool hasMovies = mCollection && !mCollection->isEmpty();

    addNew = menu.addAction(MvdMainWindow::tr("New movie..."));

    menu.addMenu(mMN_FileImport);
    mMN_FileImport->setEnabled(!mMN_FileImport->isEmpty());

    menu.addMenu(mMN_FileExport);
    mMN_FileExport->setEnabled(!mMN_FileExport->isEmpty() && hasMovies);

    if (!mMN_FileExport->isEmpty() && hasMovies)
        updatePluginsMenu();

    QModelIndexList selected = mSelectionModel->selectedRows();
    mvdid currentId = movieIndexToId(index);
    bool movieMenuAdded = false;
    if (currentId != MvdNull) {

        MvdMovie movie = mCollection->movie(currentId);
        if (movie.isValid()) {
            if (!menu.isEmpty())
                menu.addSeparator();
            movieMenuAdded = true;
            QString title = q->fontMetrics().elidedText(movie.validTitle(), Qt::ElideMiddle, 300);
            editCurrent = menu.addAction(MvdMainWindow::tr("Edit \"%1\"", "Edit movie").arg(title));
            deleteCurrent = menu.addAction(MvdMainWindow::tr("Delete \"%1\"", "Delete movie").arg(title));
        }
    }


    if (selected.size() > 1) {
        if (!menu.isEmpty())
            menu.addSeparator();
        editSelected = menu.addAction(MvdMainWindow::tr("Edit selected movies"));
        deleteSelected = menu.addAction(MvdMainWindow::tr("Delete selected movies"));
    } else if (!selected.isEmpty() && !movieMenuAdded) {
        QModelIndex selectedIndex = selected.first();
        if (selectedIndex.isValid()) {
            mvdid id = mFilterModel->data(selectedIndex, Movida::IdRole).toUInt();
            if (id != MvdNull) {
                MvdMovie movie = mCollection->movie(id);
                if (movie.isValid()) {
                    if (!menu.isEmpty())
                        menu.addSeparator();
                    currentIsSelected = true;
                    movieMenuAdded = true;
                    QString title = q->fontMetrics().elidedText(movie.validTitle(), Qt::ElideMiddle, 300);
                    editCurrent = menu.addAction(MvdMainWindow::tr("Edit \"%1\"", "Edit movie").arg(title));
                    deleteCurrent = menu.addAction(MvdMainWindow::tr("Delete \"%1\"", "Delete movie").arg(title));
                }
            }
        }
    }

    if (sender() == mSmartView && mAG_ViewSort && mMovieModel->rowCount()) {
        if (!menu.isEmpty())
            menu.addSeparator();
        QMenu *sortMenu = menu.addMenu(MvdMainWindow::tr("Sort"));
        sortMenu->addActions(mAG_ViewSort->actions());
        sortMenu->addSeparator();
        sortMenu->addAction(mA_ViewSortDescending);
    }

    QAction *res = menu.exec(QCursor::pos());
    if (!res)
        return;

    if (res == addNew) {
        q->addMovie();
    } else if (res == editCurrent) {
        q->editMovie(currentIsSelected ? selected.first() : index);
    } else if (res == deleteCurrent) {
        q->removeMovie(currentIsSelected ? selected.first() : index);
    } else if (res == deleteSelected) {
        q->removeMovies(selected);
    } else if (res == editSelected) {
        q->editSelectedMovies();
    }
}

void MvdMainWindow::Private::movieViewToggled(QAction *a)
{
    if (a == mA_ViewModeSmart) {
        mMainViewStack->setCurrentWidget(mSmartView);
        mA_ViewModeZoom->setEnabled(true);
    } else if (a == mA_ViewModeTree) {
        mMainViewStack->setCurrentWidget(mTreeView);
        mA_ViewModeZoom->setEnabled(false);
    }
}

void MvdMainWindow::Private::sortActionTriggered(QAction *a)
{
    if (!a)
        a = mAG_ViewSort->checkedAction();

    Qt::SortOrder so = mA_ViewSortDescending && mA_ViewSortDescending->isChecked() ? Qt::DescendingOrder : Qt::AscendingOrder;
    mTreeView->sortByColumn(a ? a->data().toInt() : (int)Movida::TitleAttribute, so);
}

//! Updates the sort actions whenever the tree view is sorted by the user
void MvdMainWindow::Private::treeViewSorted(int)
{
    if (!mAG_ViewSort)
        return;

    Movida::MovieAttribute attrib = mFilterModel->sortAttribute();
    QList<QAction *> alist = mAG_ViewSort->actions();
    for (int i = 0; i < alist.size(); ++i) {
        QAction *a = alist.at(i);
        if ((Movida::MovieAttribute)a->data().toInt() == attrib) {
            a->setChecked(true);
            break;
        }
    }

    if (mA_ViewSortDescending)
        mA_ViewSortDescending->setChecked(mFilterModel->sortOrder() == Qt::DescendingOrder);
}

void MvdMainWindow::Private::setMoviePoster(mvdid movieId, const QUrl &url)
{
    if (movieId == MvdNull)
        return;
    MvdMovie movie = mCollection->movie(movieId);
    if (!movie.isValid())
        return;

    QString t = movie.validTitle();

    if (url.scheme() == QLatin1String("file")) {
        QString f = url.toLocalFile();
        f = mCollection->addImage(f, MvdMovieCollection::MoviePosterImage);
        if (!f.isEmpty()) {
            movie.setPoster(f);
            mCollection->updateMovie(movieId, movie);
            q->statusBar()->showMessage(MvdMainWindow::tr("A new movie poster has been set for '%1'.").arg(t));
        } else q->statusBar()->showMessage(MvdMainWindow::tr("Failed to set a movie poster set for '%1'.").arg(t));

    } else {
        // Download required
        if (!mHttp) {
            mHttp = new QHttp(q);
            connect(mHttp, SIGNAL(requestFinished(int, bool)), SLOT(httpRequestFinished(int, bool)));
        }

        mHttp->setHost(url.host(), url.port(80));

        QString location = url.path();
        if (url.hasQuery())
            location.append("?").append(url.encodedQuery());

        QTemporaryFile *tempFile = new QTemporaryFile(paths().tempDir());
        if (!tempFile->open()) {
            q->statusBar()->showMessage(MvdMainWindow::tr("Failed to set a movie poster set for '%1'.").arg(t));
            return;
        }

        q->statusBar()->showMessage(MvdMainWindow::tr("Downloading movie poster for '%1'.").arg(t));
        iLog() << "Downloading " << location << " from host " << url.host().append(":").append(url.port(80));
        int requestId = mHttp->get(location, tempFile);

        RemoteRequest rr;
        rr.requestType = RemoteRequest::MoviePoster;
        rr.url = url;
        rr.requestId = requestId;
        rr.data = t;
        rr.tempFile = tempFile;
        rr.target = movieId;
        mPendingRemoteRequests.append(rr);
    }
}

void MvdMainWindow::Private::httpRequestFinished(int id, bool error)
{
    RemoteRequest rr;

    for (int i = 0; i < mPendingRemoteRequests.size(); ++i) {
        if (mPendingRemoteRequests.at(i).requestId == id) {
            rr = mPendingRemoteRequests.takeAt(i);
            break;
        }
    }

    if (rr.requestType == RemoteRequest::Invalid)
        return;

    switch (rr.requestType) {
        case RemoteRequest::MoviePoster:
        {
            MvdMovie m = mCollection->movie(rr.target);
            if (!m.isValid()) {
                wLog() << "Failed to set movie poster for " << rr.data.toString() << " - invalid movie ID.";
                delete rr.tempFile;
                return;
            }

            if (error) {
                wLog() << "Download failed. Url: " << rr.url.toString();
                q->statusBar()->showMessage(MvdMainWindow::tr("Failed to download movie poster for '%1'.").arg(rr.data.toString()));
                delete rr.tempFile;
                return;
            }

            QString s = mCollection->addImage(rr.tempFile->fileName());
            if (s.isEmpty()) {
                q->statusBar()->showMessage(MvdMainWindow::tr("Failed to set a movie poster set for '%1'.").arg(rr.data.toString()));
            } else {
                m.setPoster(s);
                mCollection->updateMovie(rr.target, m);
                q->statusBar()->showMessage(MvdMainWindow::tr("A new movie poster has been set for '%1'.").arg(rr.data.toString()));
            }

            delete rr.tempFile;
        }

        default:
            ;
    }
}

Q_DECLARE_METATYPE(MvdCollectionLoader::Info);
