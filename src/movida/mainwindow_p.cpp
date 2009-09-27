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
#include "mvdcore/utils.h"

#ifdef _MVD_DEBUG
#include "mvdcore/templatecache.h"
#endif

#include "mvdshared/browserview.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QShortcut>
#include <QtGui/QStackedWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QStyleFactory>
#include <QtGui/QToolBar>
#include <QtNetwork/QHttp>

using namespace Movida;

namespace {
struct PluginAction {
    QString name;
    int type;
};

enum TempDirType {
    RootTempDir = 0,
    CollectionTempDir,
    CollectionDataDir,
    TemplatesTempDir
};
}
Q_DECLARE_METATYPE(::PluginAction);


void MvdMainWindow::Private::setupUi()
{
    QStyle *baseStyle = QApplication::style();
    QApplication::setStyle(new MvdProxyStyle(baseStyle));

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
    mInfoPanel->hide();
    layout->addWidget(mInfoPanel, 2, 0);

    mMainViewStack = new QStackedWidget;
    mMainViewStack->setFrameShadow(QFrame::Raised);
    layout->addWidget(mMainViewStack, 1, 0);

    q->statusBar()->setSizeGripEnabled(false);

    // Share filter proxy model
    mFilterModel = new MvdFilterProxyModel(q);
    mFilterModel->setSourceModel(mMovieModel);
    mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mFilterModel->setFilterRole(Movida::FilterRole);
    mFilterModel->setSortRole(Movida::SortRole);

    QAbstractItemView::EditTriggers editTriggers =
        (QAbstractItemView::EditTriggers) QAbstractItemView::AllEditTriggers;
    editTriggers &= ~QAbstractItemView::CurrentChanged;
    editTriggers &= ~QAbstractItemView::SelectedClicked;

    mSmartView = new MvdSmartView(q);
    mSmartView->setObjectName("movie-smart-view");
    mSmartView->setModel(mFilterModel);
    mSmartView->setSelectionBehavior(QAbstractItemView::SelectRows);
    mSmartView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mSmartView->setEditTriggers(editTriggers);

    mTreeView = new MvdMovieTreeView(q);
    mTreeView->setObjectName("movie-tree-view");
    mTreeView->setModel(mFilterModel);
    mTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mTreeView->setDragEnabled(true);
    mTreeView->setEditTriggers(editTriggers);

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
    connect(mSharedDataEditor, SIGNAL(itemActivated(int,bool)), this, SLOT(sharedDataEditorActivated(int,bool)));

    QAbstractItemView *sharedDataEditorView = mSharedDataEditor->view();
    sharedDataEditorView->setEditTriggers(editTriggers);
    sharedDataEditorView->setSelectionBehavior(QAbstractItemView::SelectRows);
    sharedDataEditorView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    mFilterWidget = new MvdFilterWidget;
    mFilterWidget->setVisible(false);
    mFilterWidget->editor()->installEventFilter(q);
    layout->addWidget(mFilterWidget, 0, 0);

    mHideFilterTimer = new QTimer(q);
    mHideFilterTimer->setInterval(5000);
    mHideFilterTimer->setSingleShot(true);

    createActions();
    createMenus();
    createToolBars();
    q->lockToolBars(true);

    mSmartView->setAcceptDrops(true);
    mTreeView->setAcceptDrops(true);

    retranslateUi();

    connect(&Movida::core(), SIGNAL(collectionCreated(MvdMovieCollection*)),
            this, SLOT(collectionCreated(MvdMovieCollection*)));
    connect(&Movida::core(), SIGNAL(pluginsUnloaded()),
            this, SLOT(pluginsUnloaded()));
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
    mMN_Plugins = menuBar->addMenu(MvdMainWindow::tr("&Plugins")); //! \todo Merge Tools & Plugins menu
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
    mMN_Collection->addAction(mA_CollDupMovie);
    mMN_Collection->addSeparator();
    mMN_Collection->addAction(mA_CollMeta);

    mMN_Tools->addAction(mA_ToolSdEditor);
    mMN_Tools->addAction(mA_ToolPref);
    mMN_Tools->addAction(mA_ToolLog);

    mMN_Help->addAction(mA_HelpAbout);
    mMN_Help->addAction(mA_HelpContents);
    mMN_Help->addAction(mA_HelpIndex);

#ifdef _MVD_DEBUG
    mMN_Tools->addSeparator();
    QMenu* toolsTempDirs = new QMenu("T&emp Directories", q);
    mMN_Tools->addMenu(toolsTempDirs);

    QAction* temp_dir_action = toolsTempDirs->addAction("Root");
    temp_dir_action->setData((int)RootTempDir);
    connect(temp_dir_action, SIGNAL(triggered()), this, SLOT(openTempDir()));

    temp_dir_action = toolsTempDirs->addAction("Collection (Temp)");
    temp_dir_action->setData((int)CollectionTempDir);
    connect(temp_dir_action, SIGNAL(triggered()), this, SLOT(openTempDir()));

    temp_dir_action = toolsTempDirs->addAction("Collection (Data)");
    temp_dir_action->setData((int)CollectionDataDir);
    connect(temp_dir_action, SIGNAL(triggered()), this, SLOT(openTempDir()));

    temp_dir_action = toolsTempDirs->addAction("Templates");
    temp_dir_action->setData((int)TemplatesTempDir);
    connect(temp_dir_action, SIGNAL(triggered()), this, SLOT(openTempDir()));
#endif
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

    text = MvdMainWindow::tr("&Delete selected movie");
    shortInfo = MvdMainWindow::tr("Delete the selected movie from the collection");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Delete");
    initAction(mA_CollRemMovie, text, shortInfo, longInfo, shortcut);

    text = MvdMainWindow::tr("&Edit selected movie");
    shortInfo = MvdMainWindow::tr("Open a dialog to edit the selected movie");
    longInfo = shortInfo;
    shortcut = MvdMainWindow::tr("Ctrl+E");
    initAction(mA_CollEdtMovie, text, shortInfo, longInfo, shortcut);

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
    connect(mA_CollDupMovie, SIGNAL(triggered()), q, SLOT(duplicateCurrentMovie()));
    connect(mA_CollMeta, SIGNAL(triggered()), q, SLOT(showCollectionMeta()));

    connect(mA_LockToolBars, SIGNAL(toggled(bool)), q, SLOT(lockToolBars(bool)));

    connect(mTreeView, SIGNAL(activated(QModelIndex)), q, SLOT(editMovie(QModelIndex)));
    connect(mTreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
        this, SLOT(movieViewSelectionChanged()));
    connect(mTreeView, SIGNAL(contextMenuRequested(QModelIndex, QContextMenuEvent::Reason)),
        q, SLOT(showMovieContextMenu(QModelIndex)));
    connect(mTreeView->header(), SIGNAL(sectionClicked(int)), this, SLOT(treeViewSorted(int)));

    connect(mSmartView, SIGNAL(activated(QModelIndex)), q, SLOT(editMovie(QModelIndex)));
    connect(mSmartView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
        this, SLOT(movieViewSelectionChanged()));
    connect(mSmartView, SIGNAL(contextMenuRequested(QModelIndex)),
        q, SLOT(showMovieContextMenu(QModelIndex)));

    connect(mMainViewStack, SIGNAL(currentChanged(int)), this, SLOT(currentViewChanged()));

    connect(mDetailsView, SIGNAL(linkClicked(QUrl)), q, SLOT(handleLinkClicked(QUrl)));

    connect(mFilterWidget, SIGNAL(hideRequest()), q, SLOT(resetFilter()));
    connect(mFilterWidget, SIGNAL(caseSensitivityChanged()), this, SLOT(filter()));
    connect(mFilterWidget, SIGNAL(booleanOperatorChanged()), this, SLOT(filter()));
    connect(mFilterWidget->editor(), SIGNAL(textChanged(QString)), this, SLOT(filter(QString)));

    connect(mHideFilterTimer, SIGNAL(timeout()), mFilterWidget, SLOT(hide()));
    connect(mInfoPanel, SIGNAL(closedByUser()), this, SLOT(infoPanelClosedByUser()));

    // Application shortcuts
    Movida::createShortcut(QKeySequence(Qt::CTRL + Qt::Key_F), q, SLOT(toggleFilterWidget()), q);
    Movida::createShortcut(QKeySequence(Qt::Key_Escape), q, SLOT(escape()), this);

    // DEBUG
#ifdef _MVD_DEBUG
    Movida::createShortcut(QKeySequence(Qt::CTRL + Qt::Key_P), q, SLOT(showPersistentMessage()), this);
    Movida::createShortcut(QKeySequence(Qt::CTRL + Qt::Key_T), q, SLOT(showTemporaryMessage()), this);
#endif
}

void MvdMainWindow::Private::loadPlugins()
{
    QList<MvdPluginInterface *> plugins = Movida::core().plugins();
    foreach (MvdPluginInterface * iface, plugins) {

        MvdPluginInterface::PluginInfo info = iface->info();
        QList<MvdPluginInterface::PluginAction> actions = iface->actions();

        //! \todo sort plugin names?
        QMenu *pluginMenu = mMN_Plugins->addMenu(info.name);

        for (int j = 0; j < actions.size(); ++j) {
            const MvdPluginInterface::PluginAction &a = actions.at(j);

            QAction *qa = createAction();
            qa->setText(a.text);
            qa->setStatusTip(a.helpText);
            qa->setIcon(a.icon);

            if (!a.shortcuts.isEmpty()) {
                const QKeySequence& ks = a.shortcuts[0];
                qa->setShortcut(ks);
            }

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

        pluginMenu->setEnabled(!pluginMenu->isEmpty());
    }
}

void MvdMainWindow::Private::movieViewSelectionChanged()
{
    const QModelIndexList list = mTreeView->selectedRows();
    const int count = list.size();
    const bool empty = count == 0;

    mA_CollRemMovie->setEnabled(!empty);
    mA_CollEdtMovie->setEnabled(!empty);
    mA_CollDupMovie->setEnabled(count == 1);

    updateActionTexts(count);
    updateBrowserView();

    q->movieSelectionChanged();
}

//! Updates the text of actions that are related to selected movies.
void MvdMainWindow::Private::updateActionTexts(int count)
{
    int action_count;
    action_count = Movida::variantValue(mA_CollEdtMovie->property("_mvd_count"), 0);
    if (action_count != count) {
        QString text, shortInfo;

        text = count > 1
            ? MvdMainWindow::tr("&Edit selected movies", "", count)
            : MvdMainWindow::tr("&Edit selected movie");
        shortInfo = count > 1
            ? MvdMainWindow::tr("Open a dialog to edit the selected movies", "", count)
            : MvdMainWindow::tr("Open a dialog to edit the selected movie");
        mA_CollEdtMovie->setText(text);
        mA_CollEdtMovie->setStatusTip(shortInfo);

        text = count > 1
            ? MvdMainWindow::tr("&Delete selected movies", "", count)
            : MvdMainWindow::tr("&Delete selected movie");
        shortInfo = count > 1
            ? MvdMainWindow::tr("Delete the selected movies from the collection", "", count)
            : MvdMainWindow::tr("Delete the selected movie from the collection");
        mA_CollRemMovie->setText(text);
        mA_CollRemMovie->setStatusTip(shortInfo);

        text = count > 1
            ? MvdMainWindow::tr("&Edit selected movies", "", count)
            : MvdMainWindow::tr("&Edit selected movie");
        shortInfo = count > 1
            ? MvdMainWindow::tr("Open a dialog to edit the selected movies", "", count)
            : MvdMainWindow::tr("Open a dialog to edit the selected movie");
        mA_CollEdtMovie->setText(text);
        mA_CollEdtMovie->setStatusTip(shortInfo);
        mA_CollEdtMovie->setIcon(count > 1 ? QIcon(":/images/mass-edit.svgz") : QIcon(":/images/edit.svgz"));
    }
}

void MvdMainWindow::Private::updateBrowserView()
{
    QList<mvdid> ids = q->selectedMovies();
    mDetailsView->showMovies(ids);
}

void MvdMainWindow::Private::timerEvent(QTimerEvent *e)
{
    Q_UNUSED(e);
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
void MvdMainWindow::Private::collectionCreated(MvdMovieCollection* c)
{
    Q_ASSERT(c);

    mMovieModel->setMovieCollection(c);
    mSharedDataModel->setMovieCollection(c);
    mDetailsView->setMovieCollection(c);

    mA_FileNew->setDisabled(true);

    connect(c, SIGNAL(modified()), this, SLOT(collectionModified()));
    connect(c, SIGNAL(metaDataChanged(int, QString)), this, SLOT(collectionMetaDataChanged(int)));
    connect(c, SIGNAL(movieAdded(mvdid)), this, SLOT(movieAdded(mvdid)));
    connect(c, SIGNAL(movieChanged(mvdid)), this, SLOT(movieChanged(mvdid)));
}

void MvdMainWindow::Private::pluginsUnloaded()
{
    mMN_Plugins->clear();
}

//! Called when the collection is modified.
void MvdMainWindow::Private::collectionModified()
{
    bool isEmpty = core().currentCollection()->isEmpty();
    bool isNewCollection = core().currentCollection()->path().isEmpty();
    bool isModified = core().currentCollection()->isModified();

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
    Q_UNUSED(id);
}

//! Called when a movie is added to the collection.
void MvdMainWindow::Private::movieAdded(mvdid id)
{
    Q_UNUSED(id);
}

/*!
    Updates the main window title.
*/
void MvdMainWindow::Private::updateCaption()
{
    QString windowTitle;

    windowTitle = core().currentCollection()->metaData(MvdMovieCollection::NameInfo);

    if (!windowTitle.isEmpty()) {
        ;
    } else if (core().currentCollection()->fileName().isEmpty()) {
        windowTitle = MvdMainWindow::tr("New Collection");
    } else {
        if (!core().currentCollection()->path().isEmpty())
            windowTitle = core().currentCollection()->path();
        else windowTitle = core().currentCollection()->fileName();
    }

    windowTitle.append(QLatin1String("[*]"));

#ifndef Q_WS_MAC
    QString appName = QApplication::applicationName();
    if (!appName.isEmpty())
        windowTitle += QLatin1String(" ") + QChar(0x2014) + QLatin1String(" ") + appName;
#endif

    q->setWindowTitle(windowTitle);
    q->setWindowModified(core().currentCollection() && core().currentCollection()->isModified());
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
    QList<MvdPluginInterface *> pluginList = Movida::core().plugins();
    foreach(MvdPluginInterface * i, pluginList)
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
    p.setValue("movida/quick-filter/use-or-operator", mFilterWidget->booleanOperator() == Movida::OrOperator);
    if (mMainViewStack->currentWidget() == mTreeView)
        p.setValue("movida/view-mode", "tree");
    else p.setValue("movida/view-mode", "smart");
    if (mSmartView->itemSize() == MvdSmartView::LargeItemSize)
        p.setValue("movida/smart-view/item-size", "large");
    else if (mSmartView->itemSize() == MvdSmartView::SmallItemSize)
        p.setValue("movida/smart-view/item-size", "small");
    else p.setValue("movida/smart-view/item-size", "medium");

    paths().removeDirectoryTree(paths().tempDir());

    if (core().currentCollection()->isModified()) {
        //! \todo Emergency save!
    }

    // This must be called last!!! Singletons should not be accessed from now on!
    Movida::core().storeStatus();
}

//! Returns false if the quick filter should not be shown after a key press (e.g. because focus is on the SD editor)
bool MvdMainWindow::Private::shouldShowQuickFilter() const
{
    if (mFilterWidget->isVisible())
        return true;

    if (core().currentCollection()->isEmpty())
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
    bool hasMovies = !core().currentCollection()->isEmpty();
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

    bool hasMovies = !core().currentCollection()->isEmpty();

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
    Movida::BooleanOperator op = mFilterWidget->booleanOperator();

    mFilterModel->setFilterCaseSensitivity(cs);
    mFilterModel->setFilterOperator(op);
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
        const int visible = mFilterModel->rowCount();
        const int total = mFilterModel->sourceModel()->rowCount();
        if (visible == total)
            q->showPersistentMessage(MvdMainWindow::tr("Filter matches all the movies. Clear or close the filter bar to reset the filter."));
        else if (visible == 1)
            q->showPersistentMessage(MvdMainWindow::tr("Showing one movie out of %1. Clear or close the filter bar to show all movies.")
                .arg(total));
        else q->showPersistentMessage(MvdMainWindow::tr("Showing %1 movies out of %2. Clear or close the filter bar to show all movies.", "", visible)
            .arg(visible).arg(total));
    } else q->hideMessages();
}

void MvdMainWindow::Private::dispatchMessage(Movida::MessageType t, const QString &m)
{
    switch (t) {
        case Movida::InformationMessage:
            q->showTemporaryMessage(m); break;

        case Movida::WarningMessage:
            q->showTemporaryMessage(m); break;

        case Movida::ErrorMessage:
            q->showTemporaryMessage(m); break;

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
bool MvdMainWindow::Private::saveCollectionDlg(bool silent)
{
    Q_ASSERT(!core().currentCollection()->isEmpty());

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
    MvdCollectionSaver::StatusCode res = saver.save(core().currentCollection(), filename);

    if (res != MvdCollectionSaver::NoError) {
        if (!silent)
            QMessageBox::warning(q, MVD_CAPTION, MvdMainWindow::tr("Failed to save the collection."));
        return false;
    }

    core().currentCollection()->setModifiedStatus(false);
    mA_FileSave->setEnabled(false);
    updateCaption();
    return true;
}

/*!
    Closes the current collection and initializes a new empty collection.
    Returns false if the collection has not been closed.
*/
bool MvdMainWindow::Private::closeCollection(bool silent, bool *error)
{
    if (error)
        *error = false;

    if (!mFilterWidget->isEmpty())
        q->resetFilter();

    int res = QMessageBox::Discard;

    bool isNewEmptyCollection = core().currentCollection()->path().isEmpty() && core().currentCollection()->isEmpty();

    if (core().currentCollection()->isModified() && !isNewEmptyCollection) {
        res = QMessageBox::question(q, MVD_CAPTION,
            MvdMainWindow::tr("The collection has been modified.\nDo you want to save the changes?"),
            QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel);
    }

    if (res == QMessageBox::Cancel)
        return false;

    if (res == QMessageBox::Save) {
        bool result;
        if (core().currentCollection()->path().isEmpty())
            result = saveCollectionDlg(silent);
        else result = q->saveCollection(silent);

        if (error)
            *error = result;

        if (!result)
            return false;
    }

    mA_FileNew->setDisabled(true);
    mA_FileSave->setDisabled(true);
    mA_FileSaveAs->setDisabled(true);

    updateCaption();

    core().currentCollection()->clearPersistentData();
    core().createNewCollection();

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
    QAction *editCurrent = 0;
    QAction *editSelected = 0;
    QAction *deleteCurrent = 0;
    QAction *deleteSelected = 0;

    const bool currentIsSelected = false;
    const bool hasMovies = !core().currentCollection()->isEmpty();
    const bool hasVisibleMovies = mFilterModel->rowCount();

//! \todo Delete/edit movie menu must be singular+plural
//! \todo Merge MassEdit menu action with Edit
//! \todo Mass editor special flag combos must use checked = on/off if all movies have (don't have) that flag
//! \todo Remove accidentally added mass editor code using QInputDialog
//! \todo Complete mass editor

    /// Selected movie(s)
    QModelIndexList selected = mSelectionModel->selectedRows();

    if (selected.size() == 1) {

        mvdid currentId = movieIndexToId(selected.at(0));
        MvdMovie movie = core().currentCollection()->movie(currentId);
        if (movie.isValid()) {
            if (!menu.isEmpty())
                menu.addSeparator();

            /*QString title = q->fontMetrics().elidedText(movie.validTitle(), Qt::ElideMiddle, 300);
            editCurrent = menu.addAction(MvdMainWindow::tr("Edit \"%1\"", "Edit movie").arg(title));
            deleteCurrent = menu.addAction(MvdMainWindow::tr("Delete \"%1\"", "Delete movie").arg(title));*/
            menu.addAction(mA_CollEdtMovie);
            menu.addAction(mA_CollRemMovie);
        }

    } else if (selected.size() > 1) {

        if (!menu.isEmpty())
            menu.addSeparator();
        /*editSelected = menu.addAction(MvdMainWindow::tr("Edit selected movies"));
        deleteSelected = menu.addAction(MvdMainWindow::tr("Delete selected movies"));*/
        menu.addAction(mA_CollEdtMovie);
        menu.addAction(mA_CollRemMovie);
    }


    /// Collection

    if (!menu.isEmpty())
        menu.addSeparator();

    menu.addAction(mA_CollAddMovie);

    menu.addMenu(mMN_FileImport);
    mMN_FileImport->setEnabled(!mMN_FileImport->isEmpty());

    menu.addMenu(mMN_FileExport);
    mMN_FileExport->setEnabled(!mMN_FileExport->isEmpty() && hasMovies);

    if (!mMN_FileExport->isEmpty() && hasMovies)
        updatePluginsMenu();


    /// Movie view

    if (q->currentMovieView() == mSmartView && hasVisibleMovies) {
        //! \todo Add common interface for movie views with a method to return (context) menu actions
        if (!menu.isEmpty())
            menu.addSeparator();
        menu.addMenu(mMN_ViewSort);
    }

    menu.exec(QCursor::pos());
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
    MvdMovie movie = core().currentCollection()->movie(movieId);
    if (!movie.isValid())
        return;

    QString t = movie.validTitle();

    if (url.scheme() == QLatin1String("file")) {
        QString f = url.toLocalFile();
        f = core().currentCollection()->addImage(f, MvdMovieCollection::MoviePosterImage);
        if (!f.isEmpty()) {
            movie.setPoster(f);
            core().currentCollection()->updateMovie(movieId, movie);
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
            MvdMovie m = core().currentCollection()->movie(rr.target);
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

            QString s = core().currentCollection()->addImage(rr.tempFile->fileName());
            if (s.isEmpty()) {
                q->statusBar()->showMessage(MvdMainWindow::tr("Failed to set a movie poster set for '%1'.").arg(rr.data.toString()));
            } else {
                m.setPoster(s);
                core().currentCollection()->updateMovie(rr.target, m);
                q->statusBar()->showMessage(MvdMainWindow::tr("A new movie poster has been set for '%1'.").arg(rr.data.toString()));
            }

            delete rr.tempFile;
        }

        default:
            ;
    }
}

Q_DECLARE_METATYPE(MvdCollectionLoader::Info);


/////////////////////////////////////////////////////////////

namespace {
    enum LogHighlightRules {
        HeaderLHRule = 0,
        TextLineLHRule,
        DateLHRule,
        InfoLHRule,
        WarningLHRule,
        ErrorLHRule,
        WarningLineLHRule,
        ErrorLineLHRule,
        PathLHRule,
        LHRuleCount // Special
    };
}

bool MvdLogSyntaxHighlighter::mRulesInitialized = false;
QVector<MvdLogSyntaxHighlighter::HighlightingRule> MvdLogSyntaxHighlighter::mHighlightingRules =
        QVector<MvdLogSyntaxHighlighter::HighlightingRule>(LHRuleCount);

MvdLogSyntaxHighlighter::MvdLogSyntaxHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    if (!mRulesInitialized) {
        mRulesInitialized = true;

        HighlightingRule rule;

        static QTextCharFormat DefaultCharFormat;
        static QFont DefaultFont = QApplication::font();
        DefaultCharFormat.setFont(DefaultFont);

        rule.format = DefaultCharFormat;
        rule.format.setFontWeight(QFont::Bold);
        mHighlightingRules[(int)HeaderLHRule] = rule;

        rule.format = DefaultCharFormat;
        mHighlightingRules[(int)TextLineLHRule] = rule;

        rule.format = DefaultCharFormat;
        rule.format.setForeground(QColor("#585858"));
        rule.format.setFontItalic(true);
        rule.pattern.setPattern("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}");
        mHighlightingRules[(int)DateLHRule] = rule;

        rule.format = DefaultCharFormat;
        rule.pattern.setPattern("INFO");
        mHighlightingRules[(int)InfoLHRule] = rule;

        rule.format = DefaultCharFormat;
        rule.format.setForeground(QColor("#FF8000"));
        rule.pattern.setPattern("WARNING");
        mHighlightingRules[(int)WarningLHRule] = rule;

        rule.format = DefaultCharFormat;
        rule.format.setForeground(QColor("#C00000"));
        rule.pattern.setPattern("ERROR");
        mHighlightingRules[(int)ErrorLHRule] = rule;

        rule.format = DefaultCharFormat;
        mHighlightingRules[(int)WarningLineLHRule] = rule;

        rule.format = DefaultCharFormat;
        mHighlightingRules[(int)ErrorLineLHRule] = rule;

        rule.format = DefaultCharFormat;
        rule.format.setForeground(QColor("#000080"));
        rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
        rule.pattern.setPattern("(?:(?:[a-z]+:(?:(?://)|\\\\)?)|(?:/))?[^/\\\\]+(?:[\\w\\. \\-$~!/\\\\])+");
        mHighlightingRules[(int)PathLHRule] = rule;
        rule.pattern.setCaseSensitivity(Qt::CaseSensitive);
        // http://host.path.xxx?=
        // c:\test\kakk a\ncas%
        // /home/blue/lalla$
        // ~/home/blue/llalla
    }
}

void MvdLogSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    // First apply the base format
    const HighlightingRule &rule = mHighlightingRules[(int)TextLineLHRule];
    setFormat(0, text.length(), rule.format);

    // Now look for the header

    static const QString headerSignature = QLatin1String("Movida log: ");
    int start = 0;
    if (text.startsWith(headerSignature)) {
        start = qMax(text.indexOf("\n"), text.length());
        const HighlightingRule &rule = mHighlightingRules[(int)HeaderLHRule];
        setFormat(0, start, rule.format);
    }

    if (start == text.length())
        return;

    for (int i = 0; i < mHighlightingRules.size(); ++i) {
        const HighlightingRule &rule = mHighlightingRules[i];
        if (i == (int)TextLineLHRule || i == (int)HeaderLHRule)
            continue;
        const QRegExp &expression = rule.pattern;
        if (expression.pattern().isEmpty())
            continue;
        int index = expression.indexIn(text, start);
        while (index >= 0) {
            int length = expression.matchedLength();
            if (!length)
                break;
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}

void MvdMainWindow::Private::escape()
{
    if (mInfoPanel->isVisible()) {
        q->hideMessages();
        return;
    }

    if (mFilterWidget->isVisible()) {
        q->resetFilter();
        return;
    }
}

void MvdMainWindow::Private::sharedDataEditorActivated(int id, bool replace)
{
    q->filterWidget()->applySharedDataFilter(QString::number(id), replace);
}


//////////////////////////////////////////////////////////////////////////


MvdProxyStyle::MvdProxyStyle(QStyle *baseStyle) :
    mBaseStyle(0)
{
    Q_ASSERT(baseStyle);
    mBaseStyle = QStyleFactory::create(baseStyle->objectName());
    Q_ASSERT(mBaseStyle);
}

void MvdProxyStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
        const QWidget *w) const
{
    mBaseStyle->drawPrimitive(pe, opt, p, w);
}

void MvdProxyStyle::drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
    const QWidget *w) const
{
    mBaseStyle->drawControl(element, opt, p, w);
}

QRect MvdProxyStyle::subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget) const
{
    return mBaseStyle->subElementRect(r, opt, widget);
}

void MvdProxyStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
    const QWidget *w) const
{
    mBaseStyle->drawComplexControl(cc, opt, p, w);
}

QSize MvdProxyStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
    const QSize &contentsSize, const QWidget *widget) const
{
    return mBaseStyle->sizeFromContents(ct, opt, contentsSize, widget);
}

int MvdProxyStyle::pixelMetric(PixelMetric pm, const QStyleOption *option, const QWidget *widget) const
{
    return mBaseStyle->pixelMetric(pm, option, widget);
}

int MvdProxyStyle::styleHint(StyleHint stylehint, const QStyleOption *opt,
    const QWidget *widget, QStyleHintReturn* returnData) const
{
    if (stylehint == QStyle::SH_ItemView_ActivateItemOnSingleClick)
        return 0;
    return mBaseStyle->styleHint(stylehint, opt, widget, returnData);
}

QStyle::SubControl MvdProxyStyle::hitTestComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
    const QPoint &pt, const QWidget *widget) const
{
    return mBaseStyle->hitTestComplexControl(cc, opt, pt, widget);
}

QRect MvdProxyStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
    SubControl sc, const QWidget *widget) const
{
    return mBaseStyle->subControlRect(cc, opt, sc, widget);
}

QPixmap MvdProxyStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *opt,
        const QWidget *widget) const
{
    return mBaseStyle->standardPixmap(standardPixmap, opt, widget);
}

QPixmap MvdProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
        const QStyleOption *opt) const
{
    return mBaseStyle->generatedIconPixmap(iconMode, pixmap, opt);
}


//////////////////////////////////////////////////////////////////////////


#ifdef _MVD_DEBUG

void MvdMainWindow::Private::openTempDir()
{
    QAction* a = qobject_cast<QAction*>(sender());
    if (!a)
        return;

    TempDirType t = (TempDirType) a->data().toInt();
    QString path;

    static const bool DontCreateDirs = true;

    if (t == RootTempDir)
        path = paths().tempDir();
    else if (t == CollectionTempDir)
        path = core().currentCollection()->metaData(MvdMovieCollection::TempPathInfo, DontCreateDirs);
    else if (t == CollectionDataDir)
        path = core().currentCollection()->metaData(MvdMovieCollection::DataPathInfo, DontCreateDirs);
    else if (t == TemplatesTempDir)
        path = tcache().cacheDirectory();

    if (!path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MvdMainWindow::Private::showPersistentMessage()
{
    if (mInfoPanel->isVisible())
        q->hideMessages();
    else q->showPersistentMessage("Persistent TEST");
}

void MvdMainWindow::Private::showTemporaryMessage()
{
    q->showTemporaryMessage("Temporary TEST");
}

#endif // _MVD_DEBUG
