/**************************************************************************
** Filename: mainwindow.cpp
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

#include "mainwindow.h"
#include "guiglobal.h"
#include "dockwidget.h"
#include "treeview.h"
#include "collectionmodel.h"
#include "smartview.h"
#include "settingsdialog.h"
#include "pathresolver.h"
#include "templatemanager.h"
#include "logger.h"
#include "core.h"
#include "settings.h"
#include "movie.h"
#include "moviecollection.h"
#include "collectionloader.h"
#include "collectionsaver.h"
#include "plugininterface.h"
#include "rowselectionmodel.h"
#include <QList>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QIcon>
#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QDockWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QtDebug>
#include <QListView>
#include <QInputDialog>
#include <QLibrary>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QShortcut>

using namespace Movida;


/*!
	\class MvdMainWindow mainwindow.h
	\ingroup Movida

	\brief The Movida main window.
*/


/*!
	Creates a new main window and initializes the application.
*/
MvdMainWindow::MvdMainWindow(QWidget* parent)
: QMainWindow(parent), mMB_MenuBar(menuBar()), mCollection(0), mMovieEditor(0)
{
	// setIconSize(QSize(16,16));

	MVD_WINDOW_ICON
	setWindowTitle("Movida - The free movie collection manager");

	mMovieModel = new MvdCollectionModel(this);

	mMainViewStack = new QStackedWidget;
	setCentralWidget(mMainViewStack);

	mSmartView = new MvdSmartView(this);
	mSmartView->setObjectName("movie-smart-view");
	mSmartView->setModel(mMovieModel);

	mTreeView = new MvdTreeView(this);
	mTreeView->setObjectName("movie-tree-view");
	mTreeView->setModel(mMovieModel);
	
	// Share selection model
	mTreeView->setSelectionModel(new MvdRowSelectionModel(mMovieModel));
	mSmartView->setSelectionModel(mTreeView->selectionModel());
	
	mMainViewStack->addWidget(mTreeView);
	mMainViewStack->addWidget(mSmartView);	

	mDetailsDock = new MvdDockWidget(tr("Details"), this);
	mDetailsDock->setObjectName("details-dock");

	addDockWidget(Qt::BottomDockWidgetArea, mDetailsDock);

	mDetailsView = new QTextBrowser;
	mDetailsView->setSearchPaths(QStringList() << 
		Movida::paths().resourcesDir().append("Templates/Movie/"));
	mDetailsDock->setWidget(mDetailsView);

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

	mA_ToolPref = new QAction(this);
	mA_ToolPref->setIcon(QIcon(":/images/32x32/configure"));
	mA_ToolLog = new QAction(this);
	mA_ToolLog->setIcon(QIcon(":/images/32x32/log"));

	mA_PluginLoad = new QAction(this);

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
	mMN_File->addMenu(mMN_FileMRU);
	mMN_File->addSeparator();
	mMN_File->addAction(mA_FileSave);
	mMN_File->addAction(mA_FileSaveAs);
	mMN_File->addSeparator();
	mMN_File->addAction(mA_FileExit);

	QMenu* movieViewMenu = mMN_View->addMenu( tr("Movie view") );
	movieViewMenu->addActions( mAG_MovieView->actions() );

	mMN_View->addAction(mA_ViewDetails);

	mMN_Coll->addAction(mA_CollAddMovie);
	mMN_Coll->addAction(mA_CollRemMovie);
	mMN_Coll->addAction(mA_CollEdtMovie);
	mMN_Coll->addAction(mA_CollDupMovie);

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


	// Other application shortcuts
	connect( new QShortcut(Qt::CTRL + Qt::Key_V, this), SIGNAL(activated()), 
		this, SLOT(cycleMovieView()) );


	// This will set tooltips and all the i18n-ed text
	retranslateUi();

	setupConnections();
	
	
	// **************** LOAD AND APPLY STORED SETTINGS ****************
	
	// Set some GUI related constants
	QHash<QString,QVariant> parameters;
	parameters.insert("movida/maximum-recent-files", 10);
	parameters.insert("movida/default-recent-files", 5);
	parameters.insert("movida/max-menu-items", 10);
	parameters.insert("movida/message-timeout-ms", 5 * 1000);
	parameters.insert("movida/poster-default-width", 70);
	parameters.insert("movida/poster-aspect-ratio", qreal(0.7));
	parameters.insert("movida/imdb-movie-url", "http://akas.imdb.com/title/tt%1");
	MvdCore::registerParameters(parameters);

	MvdCore::loadStatus();
	MvdSettings& p = Movida::settings();

	// Set default settings
	p.setDefaultValue("movida/maximum-recent-files", 5);
	p.setDefaultValue("movida/confirm-delete-movie", true);

	p.setDefaultValue("movida/directories/use-last-collection", true);
	p.setDefaultValue("movida/movie-list/initials", false);

	// Initialize core library && load user settings
	QStringList recentFiles = p.value("movida/recent-files").toStringList();
	int max = p.value("movida/maximum-recent-files").toInt();
	int recentFilesCount = recentFiles.size();
	while (recentFiles.size() > max)
		recentFiles.removeLast();
	if (recentFilesCount != recentFiles.size())
		p.setValue("movida/recent-files", recentFiles);

	mA_FileOpenLast->setDisabled(recentFiles.isEmpty());

	QByteArray ba = p.value("movida/appearance/main-window-state").toByteArray();
	if (!ba.isEmpty())
		restoreState(ba);

	resize(p.value("movida/appearance/main-window-size", QSize(640, 480)).toSize());
	move(p.value("movida/appearance/main-window-pos", QPoint(200, 100)).toPoint());

	bool bl;
	
	bl = p.value("movida/appearance/start-maximized").toBool();
	if (bl)
		setWindowState(Qt::WindowMaximized);
	
	// a new empty collection is always open at startup
	mA_FileNew->setDisabled(true);
	mA_FileSave->setDisabled(true);
	mA_FileSaveAs->setDisabled(true);

	movieViewSelectionChanged();
	
	/*! \todo option for automatically opening the last collection or a spec collection */

	statusBar()->showMessage(tr("Movida is ready!"));
	iLog() << "Movida is ready!";

	/*QFile qss(":/qss/coffee.qss");
	qss.open(QIODevice::ReadOnly);
	qApp->setStyleSheet(QLatin1String(qss.readAll()));*/


	// **************** LOAD PLUGINS ****************
	loadPlugins();
}

/*!
	\todo Review evtl. needed deletes
*/
MvdMainWindow::~MvdMainWindow()
{
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
	mA_FileExit->setShortcut( tr( "Esc" ) );

	dockViewsToggled();

	mA_SmartView->setText( tr("&Tiles") );
	mA_TreeView->setText( tr("&Details") );
		
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

	mA_ToolPref->setText( tr( "&Preferences" ) );
	mA_ToolPref->setToolTip( tr( "Configure Movida" ) );
	mA_ToolPref->setWhatsThis( tr( "Configure Movida" ) );
	mA_ToolPref->setStatusTip( tr( "Configure Movida" ) );

	mA_ToolLog->setText( tr( "Show &Log" ) );
	mA_ToolLog->setToolTip( tr( "Shows the application log file" ) );
	mA_ToolLog->setWhatsThis( tr( "Shows the application log file" ) );
	mA_ToolLog->setStatusTip( tr( "Shows the application log file" ) );

	mA_PluginLoad->setText( tr( "&Reload plugins" ) );
	mA_PluginLoad->setToolTip( tr( "Reloads all the available plugins" ) );
	mA_PluginLoad->setWhatsThis( tr( "Reloads all the available plugins" ) );
	mA_PluginLoad->setStatusTip( tr( "Reloads all the available plugins" ) );

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
	mMN_FileMRU->setTitle( tr( "&Recent files" ) );
	mMN_View->setTitle( tr( "&View" ) );
	mMN_Coll->setTitle( tr( "&Collection" ) );
	mMN_Tool->setTitle( tr( "&Tools" ) );
	mMN_Plugins->setTitle( tr("&Plugins") );
	mMN_Help->setTitle( tr( "&Help" ) );
	
	mDetailsDock->setWindowTitle( tr("Details") );
	
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
	connect ( mMN_File, SIGNAL(aboutToShow()), this, SLOT(updateRecentFilesMenu()) );

	connect ( mA_CollAddMovie, SIGNAL( triggered() ), this, SLOT ( addMovie() ) );
	connect ( mA_CollRemMovie, SIGNAL( triggered() ), this, SLOT ( removeCurrentMovie() ) );
	connect ( mA_CollEdtMovie, SIGNAL( triggered() ), this, SLOT ( editCurrentMovie() ) );
	connect ( mA_CollDupMovie, SIGNAL( triggered() ), this, SLOT ( duplicateCurrentMovie() ) );

	connect ( mA_PluginLoad, SIGNAL( triggered() ), this, SLOT ( loadPlugins() ) );

	connect ( mTreeView, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT ( editMovie(const QModelIndex&) ) );
	connect( mTreeView->selectionModel(), SIGNAL( selectionChanged(const QItemSelection&, const QItemSelection&) ), 
		this, SLOT( movieViewSelectionChanged() ) );
	connect( mTreeView, SIGNAL( contextMenuRequested(const QModelIndex&, QContextMenuEvent::Reason) ), this, SLOT( showMovieContextMenu(const QModelIndex&) ) );
	connect( mSmartView, SIGNAL( contextMenuRequested(const QModelIndex&) ), this, SLOT( showMovieContextMenu(const QModelIndex&) ) );
}

/*!
	Shows the preferences dialog.
	\todo maybe dialog pages should better not be added here
*/
void MvdMainWindow::showPreferences()
{
	MvdSettingsDialog sd(this);
	connect(&sd, SIGNAL(externalActionTriggered(const QString&, const QVariant&)),
		this, SLOT(externalActionTriggered(const QString&, const QVariant&)) );
	sd.exec();
}

/*!
	Shows the application log.
	\todo Write some sort of log viewer.
*/
void MvdMainWindow::showLog()
{
	QFileInfo fi(paths().logFile());
	QTextBrowser* viewer = new QTextBrowser;
	viewer->setAcceptRichText(false);
	viewer->setAttribute(Qt::WA_DeleteOnClose, true);
	viewer->setWindowFlags(Qt::Tool);
	viewer->setWindowModality(Qt::NonModal);
	viewer->setSearchPaths(QStringList() << fi.absolutePath());
	viewer->setSource(QUrl(fi.fileName()));
	viewer->resize(640, 480);
	viewer->show();
}

/*!
	Handles side views toggling.
*/
void MvdMainWindow::dockViewsToggled()
{
	mA_ViewDetails->setText( mDetailsDock->windowTitle() );
	mA_ViewDetails->setToolTip( mDetailsDock->windowTitle() );
	mA_ViewDetails->setWhatsThis( mDetailsDock->windowTitle() );
	mA_ViewDetails->setStatusTip( mDetailsDock->windowTitle() );
}

/*!
	Stores preferences and application status.
*/
void MvdMainWindow::closeEvent(QCloseEvent* e)
{
	if (mCollection && mCollection->isModified())
	{
		if (!closeCollection())
		{
			e->ignore();
			return;
		}
	}

	MvdSettings& p = Movida::settings();
	p.setValue("movida/appearance/main-window-state", saveState());
	p.setValue("movida/appearance/start-maximized", isMaximized());
	p.setValue("movida/appearance/main-window-size", size());
	p.setValue("movida/appearance/main-window-pos", pos());

	MvdCore::storeStatus();
}

/*!
	Re-opens a recently opened archive.
*/
void MvdMainWindow::openRecentFile(QAction* a)
{
	if (a == 0)
		return;
	
	if (!closeCollection())
		return;
	
	QString file = a->data().toString();
	
	if (!loadCollection(file))
	{
		//! \todo Remove files that could not be opened from MRU
	}
}

/*!
	Adds a new item to the MRU files menu.
*/
void MvdMainWindow::addRecentFile(const QString& file)
{
	QFileInfo fi(file);
	QStringList list = Movida::settings().value("movida/recent-files").toStringList();

	// avoid duplicate entries
	for (int i = 0; i < list.size(); ++i)
	{
		if (fi == QFileInfo(list.at(i)))
			return;
	}
	
	list.append(file);
	
	int max = Movida::settings().value("movida/maximum-recent-files").toInt();
	while (list.size() > max)
		list.removeLast();
	
	Movida::settings().value("movida/recent-files", list).toStringList();

	mA_FileOpenLast->setDisabled(list.isEmpty());
}

/*!
	Refreshes the MRU files menu removing non existing files.
*/
void MvdMainWindow::updateRecentFilesMenu()
{
	QStringList list = Movida::settings().value("movida/recent-files").toStringList();
	
	// Remove missing files
	bool updateList = false;
	for (int i = 0; i < list.size(); ++i)
	{
		if (!QFile::exists(list.at(i)))
		{
			list.removeAt(i);
			updateList = true;
		}
	}

	if (updateList)
		Movida::settings().setValue("movida/recent-files", list);

	mMN_FileMRU->setDisabled(list.isEmpty());
	mMN_FileMRU->clear();
	
	for (int i = 0; i < list.size(); ++i)
	{
		//! \todo elide long filenames in MRU menu
		QAction* a = mMN_FileMRU->addAction(QString("&%1 %2").arg(i).arg(list.at(i)));
		a->setData(QVariant(list.at(i)));
	}
}

/*!
	Updates the main window title.
*/
void MvdMainWindow::updateCaption()
{
	/*! \todo setting: show full path on titlebar */

	QString txt;
	
	if (mCollection == 0 || mCollection->fileName().isEmpty())
	{
		txt = QString("%1 - %4").arg(MVD_CAPTION).arg(tr("New Collection"));
	}
	else
	{
		if (!mCollection->path().isEmpty())
			txt = QString("%1 - %4").arg(MVD_CAPTION).arg(mCollection->path());
		else txt = QString("%1 - %4").arg(MVD_CAPTION).arg(mCollection->fileName());
	}

	if (mCollection != 0 && mCollection->isModified())
		txt.append("*");

	setWindowTitle(txt);
}

/*!
	Closes the current collection and initializes a new empty collection.
	Returns false if the collection has not been closed.
*/
bool MvdMainWindow::closeCollection()
{
	if (mCollection == 0)
		return true;
	
	bool isNewEmptyCollection = mCollection->path().isEmpty() && mCollection->isEmpty();

	if (mCollection->isModified() && !isNewEmptyCollection)
	{
		int res = QMessageBox::question(this, MVD_CAPTION, 
			tr("The collection has been modified.\nSave changes?"),
			QMessageBox::Yes, QMessageBox::No, QMessageBox::Abort|QMessageBox::Escape);
		
		if (res == QMessageBox::Abort)
			return false;

		if (res == QMessageBox::Yes)
		{
			bool result;
			if (mCollection->path().isEmpty())
				result = saveCollectionDlg();
			else result = saveCollection();

			if (!result)
				return false;
		}
	}
	
	mMovieModel->setMovieCollection(0);

	mCollection->clearPersistentData();

	delete mCollection;
	mCollection = 0;
	
	mA_FileNew->setDisabled(true);
	mA_FileSave->setDisabled(true);
	mA_FileSaveAs->setDisabled(true);
	
	updateCaption();
	movieViewSelectionChanged();

	return true;
}

/*!
	Loads a new collection from a file.
*/
bool MvdMainWindow::loadCollection(const QString& file)
{
	if (!closeCollection())
		return true;

	/*! \todo add thread to load coll. and (_IMPORTANT_) lock gui! */
	
	if (mCollection == 0)
		createNewCollection();

	// Avoid the colelctionModified() signal emission.
	mCollection->setModifiedStatus(true);

	MvdCollectionLoader::StatusCode res = MvdCollectionLoader::load(mCollection, file);
	mCollection->setModifiedStatus(false);

	if (res != MvdCollectionLoader::NoError)
	{
		delete mCollection;
		mCollection = 0;
		QMessageBox::warning(this, MVD_CAPTION, tr("Failed to load the collection."));
		return false;
	}
	
	// Update GUI controls
	collectionModified();

	return true;
}

/*!
	Opens a file selection dialog.
	Returns false if the collection has not been loaded 
	(this is not necessarily an error).
*/
bool MvdMainWindow::loadCollectionDlg()
{
	if (!closeCollection())
		return false;
	
	MvdSettings& p = Movida::settings();

	QString lastDir = p.value("movida/directories/last-collection").toString();

	QString file = QFileDialog::getOpenFileName(
		this, MVD_CAPTION, 
		lastDir, 
		tr("Movida Movie Collection (*.mmc)")
	);
	
	if (file.isNull())
		return false;
	
	int sep = file.lastIndexOf("/");
	if (sep > 0)
	{
		if (p.value("movida/directories/use-last-collection").toBool())
			p.setValue("movida/directories/last-collection", file.left(sep));
	}
	
	addRecentFile(file);
	return loadCollection(file);
}

/*!
	Shows a file selection dialog and saves the collection.
	Returns false if the collection has not been saved 
	(this is not necessarily an error).
*/
bool MvdMainWindow::saveCollectionDlg()
{
	Q_ASSERT(mCollection != 0);
	Q_ASSERT(!mCollection->isEmpty());

	QString lastDir = Movida::settings().value("movida/directories/last-collection").toString();

	QString filename = QFileDialog::getSaveFileName(this, MVD_CAPTION, lastDir, "*.mmc");
	if (filename.isEmpty())
		return false;

	if (!filename.endsWith(".mmc"))
		filename.append(".mmc");

	int sep = filename.lastIndexOf("/");
	if (sep > 0)
	{
		if (Movida::settings().value("movida/directories/use-last-collection").toBool())
			Movida::settings().setValue("movida/directories/last-collection", filename.left(sep));
	}

	MvdCollectionSaver::ErrorCode res = MvdCollectionSaver::save(mCollection, filename);

	if (res != MvdCollectionSaver::NoError)
	{
		QMessageBox::warning(this, MVD_CAPTION, tr("Failed to save the collection."));
		return false;
	}

	mCollection->setModifiedStatus(false);
	mA_FileSave->setEnabled(false);
	updateCaption();
	return true;
}

/*!
	Saves the current collection.
*/
bool MvdMainWindow::saveCollection()
{
	Q_ASSERT(mCollection != 0);
	Q_ASSERT(!mCollection->isEmpty());

	MvdCollectionSaver::ErrorCode res = MvdCollectionSaver::save(mCollection);
	if (res != MvdCollectionSaver::NoError)
	{
		QMessageBox::warning(this, MVD_CAPTION, tr("Failed to save the collection."));
		return false;
	}

	mCollection->setModifiedStatus(false);
	mA_FileSave->setEnabled(false);
	updateCaption();

	return true;
}

/*!
	Re-opens the last closed collection.
*/
void MvdMainWindow::loadLastCollection()
{
	QStringList recentFiles = 
		Movida::settings().value("movida/recent-files").toStringList();

	if (recentFiles.isEmpty())
		return;
		
	loadCollection(recentFiles.at(0));
}

/*!
	\internal
	Creates a new collection. Sets the collection as the view's model and
	connects the proper signals and slots.
*/
void MvdMainWindow::createNewCollection()
{
	Q_ASSERT(mCollection == 0);

	mCollection = new MvdMovieCollection();
	mMovieModel->setMovieCollection(mCollection);

	connect (mCollection, SIGNAL(modified()), this, SLOT(collectionModified()) );
	connect (mCollection, SIGNAL(movieChanged(mvdid)), this, SLOT(movieChanged(mvdid)) );
}

void MvdMainWindow::collectionModified()
{
	updateCaption();
	bool isEmpty = mCollection->isEmpty();
	bool isNewCollection = mCollection->path().isEmpty();
	bool isModified = mCollection->isModified();

	mA_FileNew->setDisabled(isNewCollection && isEmpty);
	mA_FileSave->setDisabled(!isModified || isNewCollection || isEmpty);
	mA_FileSaveAs->setDisabled(isEmpty);

	updateDetailsView();
}

/*!
	Shows a dialog to add a new movie.
*/
void MvdMainWindow::addMovie()
{
	if (mCollection == 0)
	{
		createNewCollection();

		mvdid catID1 = mCollection->smd().addItem(MvdSdItem(Movida::TagRole, "Terry Gilliam Collection"));
		mvdid catID2 = mCollection->smd().addItem(MvdSdItem(Movida::TagRole, "Loaned to Mike"));

		mvdid idGEN1 = mCollection->smd().addItem(MvdSdItem(Movida::GenreRole, "Science fiction"));
		mvdid idGEN2 = mCollection->smd().addItem(MvdSdItem(Movida::GenreRole, "Comedy"));

		mvdid idPER1 = mCollection->smd().addItem(MvdSdItem(Movida::PersonRole, "Robert DeNiro"));
		mvdid idPER2 = mCollection->smd().addItem(MvdSdItem(Movida::PersonRole, "Nicole Kidman"));

		MvdMovie m;
		m.setTitle("Paura E Delirio A Las Vegas");
		m.setOriginalTitle("Fear And Loathing In Las Vegas");
		m.setEdition("Collector's Edition");
		m.setProductionYear("1992");
		m.setReleaseYear("2004");
		m.setRunningTime(112);
		m.setRating(4);
		m.addTag(catID1);
		m.addTag(catID2);
		m.addGenre(idGEN1);
		m.addGenre(idGEN2);
		m.addActor(idPER2);
		m.addActor(idPER1, QStringList() << "Ms. Katsuma" << "Hacker" << "Naima");
		m.addDirector(idPER1);
		m.addUrl(MvdUrl("http://www.imdb.com", "IMDb", true));
		m.addUrl(MvdUrl("http://www.imdb2.com", "Yet another url", true));
		m.setPlot("QDialogButtonBox now sets the default button to the first button with   the Accept role if no other button has explicitly been set as the default when it is shown. This is to stop a regression where using thec autoDefault property with the Mac and Cleanlooks styles would set the Cancel button as the default.");
		m.setNotes("QDialogButtonBox now sets the default button to the first button with   the Accept role if no other button has explicitly been set as the default when it is shown. This is to stop a regression where using thec autoDefault property with the Mac and Cleanlooks styles would set the Cancel button as the default.");

		mvdid id = mCollection->addMovie(m);

		MvdMovieEditor pd(mCollection, this);

		pd.setMovie(id);
		pd.exec();
	}
	else
	{
		MvdMovieEditor pd(mCollection, this);
		pd.exec();
	}

	// Select the new movie
	mTreeView->selectIndex( mMovieModel->index( mMovieModel->rowCount()-1, 0 ) );
	collectionModified();
}

void MvdMainWindow::newCollection()
{
	if (mCollection == 0)
		return;

	if (closeCollection())
		mA_FileNew->setDisabled(true);
}

void MvdMainWindow::editPreviousMovie()
{
	QModelIndex prev = mTreeView->indexAbove(mTreeView->selectedIndex());
	editMovie(prev);
}

void MvdMainWindow::editCurrentMovie()
{
	QModelIndexList list = mTreeView->selectedRows();
	if (list.isEmpty())
		return;
	if (list.size() == 1)
		editMovie(list.at(0));
	else QMessageBox::warning(this, MVD_CAPTION, "Multiple movie editing not implemented yet");
}

void MvdMainWindow::editNextMovie()
{
	QModelIndex next = mTreeView->indexBelow(mTreeView->selectedIndex());
	editMovie(next);
}

void MvdMainWindow::duplicateCurrentMovie()
{
	mvdid id = modelIndexToId(mTreeView->selectedIndex());
	if (id == 0)
		return;

	MvdMovie movie = mCollection->movie(id);
	if (!movie.isValid())
		return;

	bool ok;
	QString text = QInputDialog::getText(this, MVD_CAPTION,
		tr("Please enter a (unique) title for the new movie:"), QLineEdit::Normal,
		movie.title(), &ok);

	if (!ok || text.isEmpty())
		return;

	//! \todo Validate title for duplicated movie
	if (text.toLower() == movie.title().toLower())
	{
		QMessageBox::warning(this, MVD_CAPTION, tr("Sorry, but the new title must be unique in this collection."));
		return;
	}

	movie.setTitle(text);
	if (mCollection->addMovie(movie) == 0)
		QMessageBox::warning(this, MVD_CAPTION, tr("Sorry but the movie could not be duplicated."));
}

void MvdMainWindow::editMovie(const QModelIndex& index)
{
	mvdid id = modelIndexToId(index);
	bool resetRequired = false;

	if (mMovieEditor.isNull())
		mMovieEditor = new MvdMovieEditor(mCollection, this);
	else
		resetRequired = true;
	
	mMovieEditor->setAttribute(Qt::WA_DeleteOnClose, true);
	
	if (!mMovieEditor->setMovie(id, true))
		return;
	
	mTreeView->setCurrentIndex(index);

	if (resetRequired)
	{
		// Reset state
		mMovieEditor->disconnect(this);
		mMovieEditor->setPreviousEnabled(false);
		mMovieEditor->setNextEnabled(false);
	}

	QModelIndex i;
	bool prev = false;
	bool next = false;

	i = mTreeView->indexAbove(index);
	if (i.isValid())
	{
		id = modelIndexToId(i);
		if (id != 0)
		{
			mMovieEditor->setPreviousEnabled(true);
			prev = true;
		}
	}

	i = mTreeView->indexBelow(index);
	if (i.isValid())
	{
		id = modelIndexToId(i);
		if (id != 0)
		{
			mMovieEditor->setNextEnabled(true);
			next = true;
		}
	}

	if (next)
		connect(mMovieEditor, SIGNAL(nextRequested()), this, SLOT(editNextMovie()));
	if (prev)
		connect(mMovieEditor, SIGNAL(previousRequested()), this, SLOT(editPreviousMovie()));

	mMovieEditor->exec();
}

quint32 MvdMainWindow::modelIndexToId(const QModelIndex& index) const
{
	QAbstractItemModel* model = mTreeView->model();
	if (model == 0)
		return 0;

	bool ok;
	quint32 id = model->data(index, Movida::IdRole).toUInt(&ok);
	return ok ? id : 0;
}

void MvdMainWindow::removeCurrentMovie()
{
	QModelIndexList list = mTreeView->selectedRows();
	if (list.isEmpty())
		return;

	removeMovies(list);
}

void MvdMainWindow::removeMovie(const QModelIndex& index)
{
	mvdid id = modelIndexToId(index);
	if (id == 0)
		return;

	bool confirmOk = Movida::settings().value("movida/confirm-delete-movie").toBool();
	if (confirmOk)
	{
		QString msg = tr("Are you sure you want to delete this movie?");

		int res = QMessageBox::question(this, MVD_CAPTION, msg,
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return;
	}

	mCollection->removeMovie(id);

	movieViewSelectionChanged();
	collectionModified();
}

void MvdMainWindow::removeMovies(const QModelIndexList& list)
{
	bool confirmOk = Movida::settings().value("movida/confirm-delete-movie").toBool();
	if (confirmOk)
	{
		QString msg = list.size() < 2 ? tr("Are you sure you want to delete this movie?")
			: tr("Are you sure you want to delete %1 movies?").arg(list.size());

		int res = QMessageBox::question(this, MVD_CAPTION, msg,
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return;
	}

	for (int i = 0; i < list.size(); ++i)
	{
		mvdid id = modelIndexToId(list.at(i));
		if (id == 0)
			continue;

		mCollection->removeMovie(id);
	}

	movieViewSelectionChanged();
	collectionModified();
}

void MvdMainWindow::movieViewSelectionChanged()
{
	QModelIndexList list = mTreeView->selectedRows();

	mA_CollRemMovie->setEnabled(!list.isEmpty());
	mA_CollEdtMovie->setEnabled(!list.isEmpty());
	mA_CollDupMovie->setEnabled(list.size() == 1);

	updateDetailsView();
}

void MvdMainWindow::updateDetailsView()
{
	QModelIndexList list = mTreeView->selectedRows();

	if (list.size() != 1)
	{
		mDetailsView->clear();
		return;
	}

	mvdid current = modelIndexToId(list.at(0));
	if (current == 0)
	{
		mDetailsView->clear();
		return;
	}

	MvdMovie movie = mCollection->movie(current);
	QString html = tmanager().movieToHtml(movie, *mCollection, "blue.xsl");
	mDetailsView->setHtml(html);
}

void MvdMainWindow::movieChanged(mvdid id)
{
	mvdid current = modelIndexToId(mTreeView->selectedIndex());

	if (current == 0)
		return;

	if (current == id)
		updateDetailsView();
}

void MvdMainWindow::showMovieContextMenu(const QModelIndex& index)
{
/// CONTEXT MENU with no selected items: how to detect it?
	if (!index.isValid())
		return;
	
	QAbstractItemModel* model = mTreeView->model();
	if (model == 0)
		return;

	bool ok;
	mvdid id = model->data(index, Movida::IdRole).toUInt(&ok);
	if (!ok || id == 0)
		return;

	MvdMovie movie = mCollection->movie(id);
	if (!movie.isValid())
		return;

	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (!senderWidget)
		return;

	QMenu menu;
	if (sender() == mSmartView)
	{
		QMenu* sortMenu = menu.addMenu(tr("Sort movies by"));
		sortMenu->addAction(tr("Title"));
		sortMenu->addAction(tr("Production year"));
		sortMenu->addAction(tr("Release year"));
	}
	
	menu.exec(QCursor::pos());
}

//! \internal
void MvdMainWindow::externalActionTriggered(const QString& id, const QVariant& data)
{
	Q_UNUSED(data);

	if (id == "clear-mru")
	{
		Movida::settings().setValue("movida/recent-files", QStringList());
		mA_FileOpenLast->setEnabled(false);
	}
	else eLog() << QString("Unregistered external action: ").append(id);
}

//! \internal
void MvdMainWindow::loadPlugins()
{
	// Clear old plugin menus if any
	mMN_Plugins->clear();

	// Load plugins
	QDir pluginDir(paths().resourcesDir().append("Plugins"));

#if defined(Q_WS_WIN)
	QString ext = "*.dll";
#elif defined(Q_WS_MAC)
	QString ext = "*.dylib";
#else
	QString ext = "*.so";
#endif

	QFileInfoList list = pluginDir.entryInfoList(QStringList() << ext);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo& fi = list[i];
		QLibrary myLib(fi.absoluteFilePath());
		if (!myLib.load())
		{
			eLog() << QString("Failed to load %1 (reason: %2)")
				.arg(fi.absoluteFilePath()).arg(myLib.errorString());
			continue;
		}

		iLog() << "Checking plugin " << fi.absoluteFilePath();

		typedef MvdPluginInterface* (*PluginInterfaceF)(QObject*);
		PluginInterfaceF pluginInterfaceF = (PluginInterfaceF) myLib.resolve("pluginInterface");
		if (!pluginInterfaceF)
			continue;

		MvdPluginInterface* iface = pluginInterfaceF(this);
		if (!iface)
			continue;

		MvdPluginInterface::PluginInfo info = iface->info();
		if (info.name.isEmpty())
		{
			wLog() << "Discarding unnamed plugin.";
			continue;
		}

		iLog() << QString("'%1' plugin loaded.").arg(info.name);

		QList<MvdPluginInterface::PluginAction> actions = iface->actions();

		if (actions.isEmpty())
			continue;

		//! \todo Check if completeBaseName() works with .so.1.xyz linux libraries!!
		QString dataStorePath = paths().resourcesDir().append("Plugins/").append(fi.completeBaseName());
		if (!QFile::exists(dataStorePath))
		{
			QDir d;
			if (!d.mkpath(dataStorePath))
			{
				eLog() << "Failed to create user data store for plugin: " << dataStorePath;
				continue;
			}
		}

		iface->setDataStore(dataStorePath, MvdPluginInterface::UserScope);
		iLog() << QString("'%1' plugin user data store created: ").arg(info.name).append(dataStorePath);

		// Create global data store (if possible)
		dataStorePath = "";// QCoreApplication::

		// Initialize plugin
		iface->init();
		iLog() << QString("'%1' plugin initialized.").arg(info.name);

		//! \todo sort plugin names?
		QMenu* pluginMenu = mMN_Plugins->addMenu(info.name);

		QSignalMapper* signalMapper = new QSignalMapper(iface);

		for (int j = 0; j < actions.size(); ++j)
		{
			const MvdPluginInterface::PluginAction& a = actions.at(j);
			QAction* qa = new QAction(this);
			qa->setText(a.text);
			qa->setStatusTip(a.helpText);

			connect( qa, SIGNAL(triggered()), signalMapper, SLOT(map()) );
			signalMapper->setMapping(qa, a.name);

			pluginMenu->addAction(qa);
		}

		connect( signalMapper, SIGNAL(mapped(const QString&)), 
			iface, SLOT(actionTriggered(const QString&)) );
	}

	// Add the RELOAD action
	if (!mMN_Plugins->isEmpty())
		mMN_Plugins->addSeparator();
	mMN_Plugins->addAction(mA_PluginLoad);	
}

void MvdMainWindow::movieViewToggled(QAction* a)
{
	if (a == mA_SmartView)
		mMainViewStack->setCurrentWidget(mSmartView);
	else
		mMainViewStack->setCurrentWidget(mTreeView);
}

void MvdMainWindow::cycleMovieView()
{
	QWidget* w = mMainViewStack->currentWidget();
	if (w == mSmartView)
		mMainViewStack->setCurrentWidget(mTreeView);
	else
		mMainViewStack->setCurrentWidget(mSmartView);
}
