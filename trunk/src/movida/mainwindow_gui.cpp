/**************************************************************************
** Filename: mainwindow_gui.cpp
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

#include "collectionmodel.h"
#include "dockwidget.h"
#include "filterproxymodel.h"
#include "filterwidget.h"
#include "guiglobal.h"
#include "infopanel.h"
#include "mainwindow.h"
#include "movietreeview.h"
#include "movietreeviewdelegate.h"
#include "movieviewlistener.h"
#include "rowselectionmodel.h"
#include "shareddataeditor.h"
#include "shareddatamodel.h"
#include "smartview.h"
#include "mvdcore/core.h"
#include "mvdcore/pathresolver.h"
#include "mvdshared/browserview.h"
#include <QDesktopServices>
#include <QDockWidget>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QProcess>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

//!
void MvdMainWindow::setupUi()
{
	setUnifiedTitleAndToolBarOnMac(true);

	QFrame* container = new QFrame;
	container->setFrameShape(QFrame::StyledPanel);
	container->setFrameShadow(QFrame::Raised);
	setCentralWidget(container);

	QGridLayout* layout = new QGridLayout(container);
	layout->setMargin(0);
	layout->setSpacing(0);
	
	mInfoPanel = new MvdInfoPanel;
	mInfoPanel->closeImmediately(); // by-pass automatic delayed hiding
	layout->addWidget(mInfoPanel, 0, 0);

	mMainViewStack = new QStackedWidget;
	mMainViewStack->setFrameShadow(QFrame::Raised);
	layout->addWidget(mMainViewStack, 1, 0);

	statusBar()->setSizeGripEnabled(false);
	
	// Share filter proxy model
	mFilterModel = new MvdFilterProxyModel(this);
	mFilterModel->setSourceModel(mMovieModel);
	mFilterModel->setDynamicSortFilter(true);
	mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	mFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	mSmartView = new MvdSmartView(this);
	mSmartView->setObjectName("movie-smart-view");
	mSmartView->setModel(mFilterModel);

	mTreeView = new MvdMovieTreeView(this);
	mTreeView->setObjectName("movie-tree-view");
	mTreeView->setModel(mFilterModel);
	mTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	mTreeView->setDragEnabled(true);

	MvdMovieTreeViewDelegate* mtvd = new MvdMovieTreeViewDelegate(this);
	mTreeView->setItemDelegateForColumn(int(Movida::RatingAttribute), mtvd);
	mTreeView->setItemDelegateForColumn(int(Movida::SeenAttribute), mtvd);
	mTreeView->setItemDelegateForColumn(int(Movida::LoanedAttribute), mtvd);
	mTreeView->setItemDelegateForColumn(int(Movida::SpecialAttribute), mtvd);
	
	// Share selection model
	mSelectionModel = new MvdRowSelectionModel(mFilterModel);
	mTreeView->setSelectionModel(mSelectionModel);
	mSmartView->setSelectionModel(mSelectionModel);

	// Install event listener
	MvdMovieViewListener* listener = new MvdMovieViewListener(this);
	listener->registerView(mTreeView);
	listener->registerView(mSmartView);
	
	mMainViewStack->addWidget(mTreeView);
	mMainViewStack->addWidget(mSmartView);

	mDetailsDock = new MvdDockWidget(tr("Details view"), this);
	mDetailsDock->setObjectName("details-dock");
	addDockWidget(Qt::RightDockWidgetArea, mDetailsDock);

	mDetailsView = new MvdBrowserView;
	mDetailsView->setControlsVisible(false);
	mDetailsDock->setWidget(mDetailsView);
	
	mSharedDataDock = new MvdDockWidget(tr("Shared data"), this);
	mSharedDataDock->setObjectName("shared-data-dock");
	addDockWidget(Qt::RightDockWidgetArea, mSharedDataDock);

	mSharedDataEditor = new MvdSharedDataEditor;
	mSharedDataEditor->setModel(mSharedDataModel);
	mSharedDataDock->setWidget(mSharedDataEditor);

	mFilterWidget = new MvdFilterWidget;
	mFilterWidget->setVisible(false);
	mFilterWidget->editor()->installEventFilter(this);
	layout->addWidget(mFilterWidget, 2, 0);

	mHideFilterTimer = new QTimer(this);
	mHideFilterTimer->setInterval(5000);
	mHideFilterTimer->setSingleShot(true);

	//! \todo Consider using Kde4 Dolphin like status bar with success or error icons.
	
	createActions();
	createMenus();
	createToolBars();
	lockToolBars(true);

	// Intercept drag & drop. Need to install on viewport or D&D events will be blocked by the view.
	mSmartView->setAcceptDrops(true);
	mSmartView->viewport()->installEventFilter(this);
	mTreeView->setAcceptDrops(true);
	mTreeView->viewport()->installEventFilter(this);

	retranslateUi();
}

void MvdMainWindow::createActions()
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
	mAG_ViewMode = new QActionGroup(this);

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

	mAG_ViewSort = new QActionGroup(this);
	connect(mAG_ViewSort, SIGNAL(triggered(QAction*)), this, SLOT(sortActionTriggered(QAction*)));
	
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

void MvdMainWindow::createMenus()
{
	mMN_File = menuBar()->addMenu(tr("&File"));
	mMN_View = menuBar()->addMenu(tr("&View"));
	mMN_Collection = menuBar()->addMenu(tr("&Collection"));
	mMN_Plugins = menuBar()->addMenu(tr("&Plugins"));
	mMN_Tools = menuBar()->addMenu(tr("&Tools"));
	mMN_Help = menuBar()->addMenu(tr("&Help"));

	mMN_FileImport = new QMenu(tr("&Import"), this);
	mA_FileImport->setMenu(mMN_FileImport);

	mMN_FileExport = new QMenu(tr("&Export"), this);
	mA_FileExport->setMenu(mMN_FileExport);

	mMN_FileMRU = new QMenu(tr("&Recent"), this);
	mA_FileRecent->setMenu(mMN_FileMRU);

	mMN_ViewSort = new QMenu(tr("&Sort by"), this);
	mA_ViewSort->setMenu(mMN_ViewSort);

	QMenu* zoomMenu = new QMenu(tr("&Zoom level"), this);
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

void MvdMainWindow::createToolBars()
{
	mTB_MainToolBar = addToolBar(tr("Main tools"));
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

/*!
	Translates the UI.
*/
void MvdMainWindow::retranslateUi()
{
	mDetailsDock->setWindowTitle( tr("Details view") );

	QString text = tr("&New Collection");
	QString shortInfo = tr("Create a new movie collection");
	QString longInfo = shortInfo;
	QString shortcut = tr("Ctrl+N");
	initAction(mA_FileNew, text, shortInfo, longInfo, shortcut);

	text = tr("&Open Collection...");
	shortInfo = tr("Open an existing movie collection");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+O");
	initAction(mA_FileOpen, text, shortInfo, longInfo, shortcut);

	text = tr("Open &Last Collection");
	shortInfo = tr("Open the last used movie collection");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+L");
	initAction(mA_FileOpenLast, text, shortInfo, longInfo, shortcut);

	text = tr("&Import");
	shortInfo = tr("Import movies from other applications or sources");
	longInfo = tr("Import movies from other applications and file formats or from external sources like the Internet.");
	shortcut.clear();
	initAction(mA_FileImport, text, shortInfo, longInfo, shortcut);

	text = tr("&Export");
	shortInfo = tr("Export movies to other formats");
	longInfo = tr("Export movies to other formats supported by other applications.");
	shortcut.clear();
	initAction(mA_FileExport, text, shortInfo, longInfo, shortcut);

	text = tr("R&ecent Files");
	shortInfo = tr("Re-open a recently used collection");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_FileRecent, text, shortInfo, longInfo, shortcut);

	text = tr("&Save Collection");
	shortInfo = tr("Save this movie collection");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+S");
	initAction(mA_FileSave, text, shortInfo, longInfo, shortcut);

	text = tr("Save Collection &As...");
	shortInfo = tr("Select a filename to save this collection");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+Shift+S");
	initAction(mA_FileSaveAs, text, shortInfo, longInfo, shortcut);

	text = tr("E&xit");
	shortInfo = tr("Exit Movida");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_FileExit, text, shortInfo, longInfo, shortcut);

	text = tr("&Smart View");
	shortInfo = tr("Show the movies using tiles");
	longInfo = tr("Show the movies using tiles that contain the movie poster and most relevant information");
	shortcut = tr("CTRL+1");
	initAction(mA_ViewModeSmart, text, shortInfo, longInfo, shortcut);

	text = tr("&List View");
	shortInfo = tr("Show the movies as a detailed list");
	longInfo = shortInfo;
	shortcut = tr("CTRL+2");
	initAction(mA_ViewModeTree, text, shortInfo, longInfo, shortcut);

	text = tr("&Zoom level");
	shortInfo = tr("Enlarge or reduce the smart view tiles");
	longInfo = tr("Enlarge or reduce the smart view tiles to show more or less information");
	shortcut.clear();
	initAction(mA_ViewModeZoom, text, shortInfo, longInfo, shortcut);

	text = tr("&Smaller tiles");
	shortInfo = tr("Reduce the smart view tiles");
	longInfo = tr("Reduce the smart view tiles to show less information");
	shortcut = tr("CTRL+-");
	initAction(mA_ViewModeZoomOut, text, shortInfo, longInfo, shortcut);

	text = tr("&Larger tiles");
	shortInfo = tr("Enlarge the smart view tiles");
	longInfo = tr("Enlarge the smart view tiles to show more information");
	shortcut = tr("CTRL++");
	initAction(mA_ViewModeZoomIn, text, shortInfo, longInfo, shortcut);

	text = mDetailsDock->windowTitle();
	shortInfo = text;
	longInfo = text;
	shortcut.clear();
	initAction(mA_ViewDetails, text, shortInfo, longInfo, shortcut);

	text = tr("&Sort by");
	shortInfo = tr("Select the attribute used to sort the movies");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_ViewSort, text, shortInfo, longInfo, shortcut);

	text = tr("&Descending");
	shortInfo = tr("Sort movies descending");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_ViewSortDescending, text, shortInfo, longInfo, shortcut);

	text = tr("&Add a new movie");
	shortInfo = tr("Add a new movie to the collection");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+M");
	initAction(mA_CollAddMovie, text, shortInfo, longInfo, shortcut);

	text = tr("&Remove selected movie");
	shortInfo = tr("Removes the selected movie from the collection");
	longInfo = shortInfo;
	shortcut = tr("Delete");
	initAction(mA_CollRemMovie, text, shortInfo, longInfo, shortcut);

	text = tr("&Edit selected movie");
	shortInfo = tr("Open a dialog to edit the selected movie");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+E");
	initAction(mA_CollEdtMovie, text, shortInfo, longInfo, shortcut);

	text = tr("&Mass edit selected movies");
	shortInfo = tr("Open a dialog to assign common properties to the selected movies");
	longInfo = shortInfo;
	shortcut = tr("Ctrl+Shift+E");
	initAction(mA_CollMedMovie, text, shortInfo, longInfo, shortcut);

	text = tr("&Duplicate selected movie");
	shortInfo = tr("Duplicate the selected movie");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_CollDupMovie, text, shortInfo, longInfo, shortcut);

	text = tr("A&bout this collection...");
	shortInfo = tr("Show or edit basic information about this collection");
	longInfo = tr("Show or edit basic information about this collection, like name, owner, contact info etc.");
	shortcut.clear();
	initAction(mA_CollMeta, text, shortInfo, longInfo, shortcut);

	text = tr("&Shared data editor");
	shortInfo = tr("View and edit shared data");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_ToolSdEditor, text, shortInfo, longInfo, shortcut);

	text = tr("&Preferences");
	shortInfo = tr("Configure Movida");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_ToolPref, text, shortInfo, longInfo, shortcut);


	text = tr("Show &Log");
	shortInfo = tr("Shows the application log file");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_ToolLog, text, shortInfo, longInfo, shortcut);

	text = tr("&Contents");
	shortInfo = tr("Open the Movida user guide");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_HelpContents, text, shortInfo, longInfo, shortcut);

	text = tr("&Index");
	shortInfo = tr("Open the Movida user guide index");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_HelpIndex, text, shortInfo, longInfo, shortcut);

	text = tr("&About Movida");
	shortInfo = tr("Show some information about Movida");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_HelpAbout, text, shortInfo, longInfo, shortcut);

	text = tr("&Lock toolbars");
	shortInfo = tr("Lock toolbars");
	longInfo = shortInfo;
	shortcut.clear();
	initAction(mA_LockToolBars, text, shortInfo, longInfo, shortcut);

	//! \todo re-translate menus and toolbars
	
	updateCaption();
}

/*!
	Signal/slot connections.
*/
void MvdMainWindow::setupConnections()
{
	connect ( mA_FileExit, SIGNAL( triggered() ), this, SLOT ( close() ) );
	connect ( mA_FileNew, SIGNAL( triggered() ), this, SLOT ( newCollection() ) );
	connect ( mA_FileOpen, SIGNAL( triggered() ), this, SLOT ( loadCollectionDlg() ) );
	connect ( mA_FileOpenLast, SIGNAL( triggered() ), this, SLOT ( loadLastCollection() ) );
	connect ( mA_FileSaveAs, SIGNAL( triggered() ), this, SLOT ( saveCollectionDlg() ) );
	connect ( mA_FileSave, SIGNAL( triggered() ), this, SLOT ( saveCollection() ) );
	
	connect ( mA_ToolPref, SIGNAL( triggered() ), this, SLOT ( showPreferences() ) );
	connect ( mA_ToolLog, SIGNAL( triggered() ), this, SLOT ( showLog() ) );
	connect ( mA_ToolSdEditor, SIGNAL( triggered() ), this, SLOT ( showSharedDataEditor() ) );

	connect ( mAG_ViewMode, SIGNAL( triggered(QAction*) ), this, SLOT ( movieViewToggled(QAction*) ) );
	connect ( mA_ViewModeZoomIn, SIGNAL( triggered() ), this, SLOT ( zoomIn() ) );
	connect ( mA_ViewModeZoomOut, SIGNAL( triggered() ), this, SLOT ( zoomOut() ) );

	connect ( mMN_FileMRU, SIGNAL(triggered(QAction*)), this, SLOT(openRecentFile(QAction*)) );
	connect ( mMN_File, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()) );
	connect ( mMN_Plugins, SIGNAL(aboutToShow()), this, SLOT(updatePluginsMenu()) );

	connect ( mA_CollAddMovie, SIGNAL( triggered() ), this, SLOT ( addMovie() ) );
	connect ( mA_CollRemMovie, SIGNAL( triggered() ), this, SLOT ( removeSelectedMovies() ) );
	connect ( mA_CollEdtMovie, SIGNAL( triggered() ), this, SLOT ( editSelectedMovies() ) );
	connect ( mA_CollMedMovie, SIGNAL( triggered() ), this, SLOT ( massEditSelectedMovies() ) );
	connect ( mA_CollDupMovie, SIGNAL( triggered() ), this, SLOT ( duplicateCurrentMovie() ) );
	connect ( mA_CollMeta, SIGNAL( triggered() ), this, SLOT ( showCollectionMeta() ) );

	connect(mA_LockToolBars, SIGNAL(toggled(bool)), this, SLOT(lockToolBars(bool)));

	connect ( mTreeView, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT ( editMovie(const QModelIndex&) ) );
	connect( mTreeView->selectionModel(), SIGNAL( selectionChanged(const QItemSelection&, const QItemSelection&) ), 
		this, SLOT( movieViewSelectionChanged() ) );
	connect( mTreeView, SIGNAL( contextMenuRequested(const QModelIndex&, QContextMenuEvent::Reason) ), this, SLOT( showMovieContextMenu(const QModelIndex&) ) );
	connect( mTreeView->header(), SIGNAL(sectionClicked(int)), this, SLOT(treeViewSorted(int)) );
	
	connect ( mSmartView, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT ( editMovie(const QModelIndex&) ) );
	connect( mSmartView->selectionModel(), SIGNAL( selectionChanged(const QItemSelection&, const QItemSelection&) ), 
		this, SLOT( movieViewSelectionChanged() ) );
	connect( mSmartView, SIGNAL( contextMenuRequested(const QModelIndex&) ), this, SLOT( showMovieContextMenu(const QModelIndex&) ) );

	connect( mMainViewStack, SIGNAL(currentChanged(int)), this, SLOT(currentViewChanged()) );

	connect( mDetailsView, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClicked(QUrl)) );
	
	connect( mFilterWidget, SIGNAL(hideRequest()), this, SLOT(resetFilter()) );
	connect( mFilterWidget, SIGNAL(caseSensitivityChanged()), this, SLOT(filter()) );
	connect( mFilterWidget->editor(), SIGNAL(textChanged(QString)), this, SLOT(filter(QString)) );

	connect( mHideFilterTimer, SIGNAL(timeout()), mFilterWidget, SLOT(hide()) );
	connect( mInfoPanel, SIGNAL(closedByUser()), SLOT(infoPanelClosedByUser()) );

	// Application shortcuts
	connect( new QShortcut(Qt::CTRL + Qt::Key_F, this), SIGNAL(activated()), this, SLOT(showFilterWidget()) );
}

#ifdef MVD_DEBUG_TOOLS

//! \internal Sets up some convenience actions for debugging and development purposes.
void MvdMainWindow::setupDebugTools()
{
	QMenu* debugMenu = new QMenu("&Debug", this);
	menuBar()->insertMenu(mMN_Help->menuAction(), debugMenu);
}

#endif // MVD_DEBUG_TOOLS

QAction* MvdMainWindow::createAction()
{
	QAction* action = new QAction(this);
	return action;
}

void MvdMainWindow::initAction(QAction* action, const QString& text, const QString& shortInfo, const QString& longInfo, const QString& shortcut)
{
	Q_ASSERT(action);

	action->setText(text);
	action->setToolTip(shortInfo);
	action->setStatusTip(shortInfo);
	action->setWhatsThis(longInfo);
	action->setShortcut(shortcut);
}

