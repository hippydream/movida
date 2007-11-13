/**************************************************************************
** Filename: mainwindow_gui.cpp
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

#include "collectionmodel.h"
#include "dockwidget.h"
#include "filterproxymodel.h"
#include "filterwidget.h"
#include "guiglobal.h"
#include "mainwindow.h"
#include "pathresolver.h"
#include "rowselectionmodel.h"
#include "smartview.h"
#include "treeview.h"
#include <QDockWidget>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextBrowser>
#include <QToolBar>
#include <QTimer>

//!
void MvdMainWindow::setupUi()
{
	QWidget* container = new QWidget;
	setCentralWidget(container);
	QGridLayout* layout = new QGridLayout(container);
	
	mMainViewStack = new QStackedWidget;
	layout->addWidget(mMainViewStack, 0, 0);
	
	// Share filter proxy model
	mFilterModel = new MvdFilterProxyModel(this);
	mFilterModel->setSourceModel(mMovieModel);
	mFilterModel->setDynamicSortFilter(true);
	mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

	mSmartView = new MvdSmartView(this);
	mSmartView->setObjectName("movie-smart-view");
	mSmartView->setModel(mFilterModel);

	mTreeView = new MvdTreeView(this);
	mTreeView->setObjectName("movie-tree-view");
	mTreeView->setModel(mFilterModel);
	
	// Share selection model
	mTreeView->setSelectionModel(new MvdRowSelectionModel(mFilterModel));
	mSmartView->setSelectionModel(mTreeView->selectionModel());
	
	mMainViewStack->addWidget(mTreeView);
	mMainViewStack->addWidget(mSmartView);	

	mDetailsDock = new MvdDockWidget(tr("Details view"), this);
	mDetailsDock->setObjectName("details-dock");

	addDockWidget(Qt::BottomDockWidgetArea, mDetailsDock);

	mDetailsView = new QTextBrowser;
	mDetailsView->setSearchPaths(QStringList() << 
		Movida::paths().resourcesDir().append("Templates/Movie/"));
	mDetailsDock->setWidget(mDetailsView);
	
	mFilterWidget = new MvdFilterWidget;
	mFilterWidget->setVisible(false);
	mFilterWidget->editor()->installEventFilter(this);
	layout->addWidget(mFilterWidget, 1, 0);

	mHideFilterTimer = new QTimer(this);
	mHideFilterTimer->setInterval(5000);
	mHideFilterTimer->setSingleShot(true);
	
	// Actions
	mA_FileNew = new QAction(this);
	mA_FileNew->setIcon(QIcon(":/images/32x32/file_new"));
	mA_FileOpen = new QAction(this);
	mA_FileOpen->setIcon(QIcon(":/images/32x32/file_open"));
	mA_FileOpenLast = new QAction(this);
	mA_FileOpenLast->setIcon(QIcon(":/images/32x32/file_openlast"));
	mA_FileSave = new QAction(this);
	mA_FileSave->setIcon(QIcon(":/images/32x32/file_save"));
	mA_FileSaveAs = new QAction(this);
	mA_FileSaveAs->setIcon(QIcon(":/images/32x32/file_saveas"));
	mA_FileExit = new QAction(this);
	mA_FileExit->setIcon(QIcon(":/images/32x32/exit"));

	mA_TreeView = new QAction(this);
	mA_TreeView->setIcon(QIcon(":/images/32x32/exit"));
	mA_TreeView->setCheckable(true);
	mA_TreeView->setChecked(true);

	mA_SmartView = new QAction(this);
	mA_SmartView->setIcon(QIcon(":/images/32x32/exit"));
	mA_SmartView->setCheckable(true);

	mAG_MovieView = new QActionGroup(this);
	mAG_MovieView->setExclusive(true);
	mAG_MovieView->addAction(mA_SmartView);
	mAG_MovieView->addAction(mA_TreeView);

	mA_ViewDetails = mDetailsDock->toggleViewAction();
	
	mA_CollAddMovie = new QAction(this);
	mA_CollAddMovie->setIcon(QIcon(":/images/32x32/add"));
	mA_CollAddMovie->setShortcut(Qt::CTRL | Qt::Key_A);
	mA_CollRemMovie = new QAction(this);
	mA_CollRemMovie->setIcon(QIcon(":/images/32x32/remove"));
	mA_CollEdtMovie = new QAction(this);
	mA_CollEdtMovie->setIcon(QIcon(":/images/32x32/edit"));
	mA_CollDupMovie = new QAction(this);
	mA_CollDupMovie->setIcon(QIcon(":/images/32x32/duplicate"));
	mA_CollMeta = new QAction(this);
	mA_CollMeta->setIcon(QIcon(":/images/32x32/metaeditor"));

	mA_ToolPref = new QAction(this);
	mA_ToolPref->setIcon(QIcon(":/images/32x32/configure"));
	mA_ToolLog = new QAction(this);
	mA_ToolLog->setIcon(QIcon(":/images/32x32/log"));

	mA_HelpContents = new QAction(this);
	mA_HelpContents->setIcon(QIcon(":/images/32x32/help"));
	mA_HelpIndex = new QAction(this);
	mA_HelpAbout = new QAction(this);
	mA_HelpAbout->setIcon(QIcon(":/images/32x32/logo"));


	// Toolbars
	mTB_File = addToolBar(tr("File"));
	mTB_File->setObjectName("file-toolbar");
	mTB_View = addToolBar(tr("View"));
	mTB_View->setObjectName("view-toolbar");
	mTB_Tool = addToolBar(tr("Tool"));
	mTB_Tool->setObjectName("tool-toolbar");
	mTB_Coll = addToolBar(tr("Collection"));
	mTB_Coll->setObjectName("collection-toolbar");
	mTB_Help = addToolBar(tr("Help"));
	mTB_Help->setObjectName("help-toolbar");


	// Menus
	mMN_File = new QMenu(this);
	mMB_MenuBar->addMenu(mMN_File);

	mMN_FileImport = new QMenu(mMN_File);

	mMN_FileMRU = new QMenu(mMN_File);

	mMN_View = new QMenu(this);
	mMB_MenuBar->addMenu(mMN_View);

	mMN_Coll = new QMenu(this);
	mMB_MenuBar->addMenu(mMN_Coll);

	mMN_Tool = new QMenu(this);
	mMB_MenuBar->addMenu(mMN_Tool);

	mMN_Plugins = new QMenu(this);
	mMB_MenuBar->addMenu(mMN_Plugins);

	mMN_Help = new QMenu(this);
	mMB_MenuBar->addMenu(mMN_Help);


	// Populate menus
	mMN_File->addAction(mA_FileNew);
	mMN_File->addSeparator();
	mMN_File->addAction(mA_FileOpen);
	mMN_File->addAction(mA_FileOpenLast);
	mMN_File->addMenu(mMN_FileImport);
	mMN_File->addMenu(mMN_FileMRU);
	mMN_File->addSeparator();
	mMN_File->addAction(mA_FileSave);
	mMN_File->addAction(mA_FileSaveAs);
	mMN_File->addSeparator();
	mMN_File->addAction(mA_FileExit);

	QMenu* movieViewMenu = mMN_View->addMenu( tr("Movie view") );
	movieViewMenu->addActions( mAG_MovieView->actions() );

	mMN_View->addAction(mA_ViewDetails);

	mMN_ViewSort = new QMenu(this);
	mMN_View->addSeparator();
	mMN_View->addMenu(mMN_ViewSort);
	
	mMN_Coll->addAction(mA_CollAddMovie);
	mMN_Coll->addAction(mA_CollRemMovie);
	mMN_Coll->addAction(mA_CollEdtMovie);
	mMN_Coll->addAction(mA_CollDupMovie);
	mMN_Coll->addSeparator();
	mMN_Coll->addAction(mA_CollMeta);

	mMN_Tool->addAction(mA_ToolPref);
	mMN_Tool->addAction(mA_ToolLog);

	mMN_Help->addAction(mA_HelpAbout);
	mMN_Help->addAction(mA_HelpContents);
	mMN_Help->addAction(mA_HelpIndex);

	// Populate toolbars
	mTB_File->addAction(mA_FileNew);
	mTB_File->addAction(mA_FileOpen);
	mTB_File->addAction(mA_FileOpenLast);
	mTB_File->addSeparator();
	mTB_File->addAction(mA_FileSave);
	mTB_File->addAction(mA_FileSaveAs);
	mTB_File->addSeparator();
	mTB_File->addAction(mA_FileExit);

	mTB_View->addAction(mA_ViewDetails);

	mTB_Coll->addAction(mA_CollAddMovie);
	mTB_Coll->addAction(mA_CollRemMovie);
	mTB_Coll->addAction(mA_CollEdtMovie);
	mTB_Coll->addAction(mA_CollDupMovie);

	mTB_Tool->addAction(mA_ToolPref);

	mTB_Help->addAction(mA_HelpContents);

	// This will set tooltips and all the i18n-ed text
	retranslateUi();
}

/*!
	Translates the UI.
*/
void MvdMainWindow::retranslateUi()
{
	mA_FileNew->setText( tr( "&New Collection" ) );
	mA_FileNew->setToolTip( tr( "Create a new movie collection" ) );
	mA_FileNew->setWhatsThis( tr( "Create a new movie collection" ) );
	mA_FileNew->setStatusTip( tr( "Create a new movie collection" ) );
	mA_FileNew->setShortcut( tr( "Ctrl+N" ) );
	mA_FileOpen->setText( tr( "&Open Collection" ) );
	mA_FileOpen->setToolTip( tr( "Open an existing movie collection" ) );
	mA_FileOpen->setWhatsThis( tr( "Open an existing movie collection" ) );
	mA_FileOpen->setStatusTip( tr( "Open an existing movie collection" ) );
	mA_FileOpen->setShortcut( tr( "Ctrl+O" ) );
	mA_FileOpenLast->setText( tr( "Open &Last Collection" ) );
	mA_FileOpenLast->setToolTip( tr( "Open the last used movie collection" ) );
	mA_FileOpenLast->setWhatsThis( tr( "Open the last used movie collection" ) );
	mA_FileOpenLast->setStatusTip( tr( "Open the last used movie collection" ) );
	mA_FileOpenLast->setShortcut( tr( "Ctrl+L" ) );
	mA_FileSave->setText( tr( "&Save Collection" ) );
	mA_FileSave->setToolTip( tr( "Save this movie collection" ) );
	mA_FileSave->setWhatsThis( tr( "Save this movie collection" ) );
	mA_FileSave->setStatusTip( tr( "Save this movie collection" ) );
	mA_FileSave->setShortcut( tr( "Ctrl+S" ) );
	mA_FileSaveAs->setText( tr( "Save Collection &As..." ) );
	mA_FileSaveAs->setToolTip( tr( "Select a filename to save this collection" ) );
	mA_FileSaveAs->setWhatsThis( tr( "Select a filename to save this collection" ) );
	mA_FileSaveAs->setStatusTip( tr( "Select a filename to save this collection" ) );
	mA_FileSaveAs->setShortcut( tr( "Ctrl+Shift+S" ) );
	mA_FileExit->setText( tr( "E&xit" ) );
	mA_FileExit->setToolTip( tr( "Exit Movida" ) );
	mA_FileExit->setWhatsThis( tr( "Exit Movida" ) );
	mA_FileExit->setStatusTip( tr( "Exit Movida" ) );

	dockViewsToggled();

	mA_SmartView->setText( tr("&Tiles") );
	mA_TreeView->setText( tr("&List") );
		
	mA_CollAddMovie->setText( tr( "&Add a new movie" ) );
	mA_CollAddMovie->setToolTip( tr( "Add a new movie to the collection" ) );
	mA_CollAddMovie->setWhatsThis( tr( "Add a new movie to the collection" ) );
	mA_CollAddMovie->setStatusTip( tr( "Add a new movie to the collection" ) );
	mA_CollRemMovie->setText( tr( "&Remove selected movie" ) );
	mA_CollRemMovie->setToolTip( tr( "Removes the selected movie from the collection" ) );
	mA_CollRemMovie->setWhatsThis( tr( "Removes the selected movie from the collection" ) );
	mA_CollRemMovie->setStatusTip( tr( "Removes the selected movie from the collection" ) );
	mA_CollEdtMovie->setText( tr( "&Edit selected movie" ) );
	mA_CollEdtMovie->setToolTip( tr( "Open a dialog to edit the selected movie" ) );
	mA_CollEdtMovie->setWhatsThis( tr( "Open a dialog to edit the selected movie" ) );
	mA_CollEdtMovie->setStatusTip( tr( "Open a dialog to edit the selected movie" ) );
	mA_CollDupMovie->setText( tr( "&Duplicate selected movie" ) );
	mA_CollDupMovie->setToolTip( tr( "Duplicate the selected movie" ) );
	mA_CollDupMovie->setWhatsThis( tr( "Duplicate the selected movie" ) );
	mA_CollDupMovie->setStatusTip( tr( "Duplicate the selected movie" ) );
	mA_CollMeta->setText( tr( "A&bout this collection..." ) );
	mA_CollMeta->setToolTip( tr( "Show or edit basic information about this collection" ) );
	mA_CollMeta->setWhatsThis( tr( "Show or edit basic information about this collection, like a name, the owner, etc." ) );
	mA_CollMeta->setStatusTip( tr( "Show or edit basic information about this collection" ) );

	mA_ToolPref->setText( tr( "&Preferences" ) );
	mA_ToolPref->setToolTip( tr( "Configure Movida" ) );
	mA_ToolPref->setWhatsThis( tr( "Configure Movida" ) );
	mA_ToolPref->setStatusTip( tr( "Configure Movida" ) );

	mA_ToolLog->setText( tr( "Show &Log" ) );
	mA_ToolLog->setToolTip( tr( "Shows the application log file" ) );
	mA_ToolLog->setWhatsThis( tr( "Shows the application log file" ) );
	mA_ToolLog->setStatusTip( tr( "Shows the application log file" ) );

	mA_HelpContents->setText( tr( "&Contents" ) );
	mA_HelpContents->setToolTip( tr( "Open the Movida user guide" ) );
	mA_HelpContents->setWhatsThis( tr( "Open the Movida user guide" ) );
	mA_HelpContents->setStatusTip( tr( "Open the Movida user guide" ) );
	mA_HelpIndex->setText( tr( "&Index" ) );
	mA_HelpIndex->setToolTip( tr( "Open the Movida user guide index" ) );
	mA_HelpIndex->setWhatsThis( tr( "Open the Movida user guide index" ) );
	mA_HelpIndex->setStatusTip( tr( "Open the Movida user guide index" ) );
	mA_HelpAbout->setText( tr( "&About Movida" ) );
	mA_HelpAbout->setToolTip( tr( "Show some information about Movida" ) );
	mA_HelpAbout->setStatusTip( tr( "Show some information about Movida" ) );
	mA_HelpAbout->setWhatsThis( tr( "Show some information about Movida" ) );

	mTB_File->setWindowTitle( tr( "File" ) );
	mTB_View->setWindowTitle( tr( "View" ) );
	mTB_Coll->setWindowTitle( tr( "Collection" ) );
	mTB_Tool->setWindowTitle( tr( "Tools" ) );
	mTB_Help->setWindowTitle( tr( "Help" ) );

	mMN_File->setTitle( tr( "&File" ) );
	mMN_FileImport->setTitle( tr( "&Import" ) );
	mMN_FileMRU->setTitle( tr( "&Recent files" ) );
	mMN_View->setTitle( tr( "&View" ) );
	mMN_ViewSort->setTitle( tr( "&Sort" ) );
	mMN_Coll->setTitle( tr( "&Collection" ) );
	mMN_Tool->setTitle( tr( "&Tools" ) );
	mMN_Plugins->setTitle( tr("&Plugins") );
	mMN_Help->setTitle( tr( "&Help" ) );
	
	mDetailsDock->setWindowTitle( tr("Details view") );
	
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

	connect ( mAG_MovieView, SIGNAL( triggered(QAction*) ), this, SLOT ( movieViewToggled(QAction*) ) );

	connect ( mDetailsDock, SIGNAL( toggled(bool) ), this, SLOT ( dockViewsToggled() ) );
	
	connect ( mMN_FileMRU, SIGNAL(triggered(QAction*)), this, SLOT(openRecentFile(QAction*)) );
	connect ( mMN_File, SIGNAL(aboutToShow()), this, SLOT(updateFileMenu()) );

	connect ( mA_CollAddMovie, SIGNAL( triggered() ), this, SLOT ( addMovie() ) );
	connect ( mA_CollRemMovie, SIGNAL( triggered() ), this, SLOT ( removeCurrentMovie() ) );
	connect ( mA_CollEdtMovie, SIGNAL( triggered() ), this, SLOT ( editCurrentMovie() ) );
	connect ( mA_CollDupMovie, SIGNAL( triggered() ), this, SLOT ( duplicateCurrentMovie() ) );
	connect ( mA_CollMeta, SIGNAL( triggered() ), this, SLOT ( showMetaEditor() ) );

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
	
	connect( mMovieModel, SIGNAL(sorted()), this, SLOT(collectionModelSorted()) );
	
	connect( mFilterWidget, SIGNAL(hideRequest()), this, SLOT(resetFilter()) );
	connect( mFilterWidget->editor(), SIGNAL(textEdited(QString)), this, SLOT(filter(QString)) );

	connect( mHideFilterTimer, SIGNAL(timeout()), mFilterWidget, SLOT(hide()) );

	// Application shortcuts
	connect( new QShortcut(Qt::CTRL + Qt::Key_V, this), SIGNAL(activated()), this, SLOT(cycleMovieView()) );
	connect( new QShortcut(Qt::CTRL + Qt::Key_F, this), SIGNAL(activated()), this, SLOT(filter()) );
}
