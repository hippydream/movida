/**************************************************************************
** Filename: mainwindow.cpp
** Revision: 1
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

#include <QList>
#include <QFile>
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

//! \todo replace with a MvdCore registered parameter
#define MVD_DEFAULT_MAX_RECENT_FILES 5

using namespace Movida;


/*!
	\class MvdMainWindow mainwindow.h
	\ingroup movida

	\brief The Movida main window.
*/


/*!
	Creates a new main window and initializes the application.
*/
MvdMainWindow::MvdMainWindow()
: QMainWindow(), mMB_MenuBar(menuBar()), mCollection(0), mMovieEditor(0)
{
	// setIconSize(QSize(16,16));

	MVD_WINDOW_ICON
	setWindowTitle("Movida - The free movie collection manager");

	mMovieModel = new MvdCollectionModel(this);

	mMovieView = new MvdTreeView(this);
	mMovieView->setObjectName("movie_view");
	mMovieView->setModel(mMovieModel);

	setCentralWidget(mMovieView);

	mDetailsDock = new MvdDockWidget(tr("Details"), this);
	mDetailsDock->setObjectName("detailsDock");

	addDockWidget(Qt::BottomDockWidgetArea, mDetailsDock);

	mDetailsView = new QTextBrowser;
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

	mA_HelpContents = new QAction(this);
	mA_HelpContents->setIcon(QIcon(":/images/32x32/help"));
	mA_HelpIndex = new QAction(this);
	mA_HelpAbout = new QAction(this);
	mA_HelpAbout->setIcon(QIcon(":/images/32x32/logo"));


	// Toolbars
	mTB_File = addToolBar(tr("File"));
	mTB_File->setObjectName("fileToolbar");
	mTB_View = addToolBar(tr("View"));
	mTB_View->setObjectName("viewToolbar");
	mTB_Tool = addToolBar(tr("Tool"));
	mTB_Tool->setObjectName("toolToolbar");
	mTB_Coll = addToolBar(tr("Collection"));
	mTB_Coll->setObjectName("collectionToolbar");
	mTB_Help = addToolBar(tr("Help"));
	mTB_Help->setObjectName("helpToolbar");


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


	//! \todo DEBUG
	QAction* testAction = new QAction("TestAction", this);
	connect( testAction, SIGNAL(triggered()), this, SLOT(testSlot()) );
	mMN_Help->addAction(testAction);

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

	setupConnections();
	
	
	// **************** LOAD AND APPLY STORED SETTINGS ****************
	
	MvdSettings& p = Movida::settings();

	// Set default settings
	p.setInt("max_recent_files", MVD_DEFAULT_MAX_RECENT_FILES, "gui_general");
	p.setBool("confirm_close_db", true, "gui_general");
	p.setBool("confirm_remove_db", true, "gui_general");
	p.setBool("use_last_archive", true, "directories");
	p.setBool("initials", false, "movie_list");
	p.setRect("main_window_rect", defaultWindowRect(), "gui_appearance");

	// Load settings
	MvdCore::initStatus();

	QByteArray ba = p.getByteArray("main_window_state", "gui_appearance");
	if (!ba.isEmpty())
		restoreState(ba);

	QRect rect = p.getRect("main_window_rect", "gui_appearance");
	if (rect.isValid())
		setGeometry(rect);

	bool ok, bl;
	
	bl = p.getBool("start_maximized", "gui_appearance", &ok);
	if (ok && bl)
		setWindowState(Qt::WindowMaximized);

	// setup recent files
	mRecentFiles = p.getStringList("recent_files", "gui_general");
	refreshRecentFilesMenu();
	
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

	connect ( mDetailsDock, SIGNAL( toggled(bool) ), this, SLOT ( dockViewsToggled() ) );
	
	connect ( mMN_FileMRU, SIGNAL(triggered(QAction*)), this, SLOT(openRecentFile(QAction*)) );
	connect ( mMN_FileMRU, SIGNAL(aboutToShow()), this, SLOT(fillRecentFilesMenu()) );

	connect ( mA_CollAddMovie, SIGNAL( triggered() ), this, SLOT ( addMovie() ) );
	connect ( mA_CollRemMovie, SIGNAL( triggered() ), this, SLOT ( removeCurrentMovie() ) );
	connect ( mA_CollEdtMovie, SIGNAL( triggered() ), this, SLOT ( editCurrentMovie() ) );
	connect ( mA_CollDupMovie, SIGNAL( triggered() ), this, SLOT ( duplicateCurrentMovie() ) );

	connect ( mMovieView, SIGNAL( doubleClicked(const QModelIndex&) ), this, SLOT ( editMovie(const QModelIndex&) ) );
	connect( mMovieView, SIGNAL( itemSelectionChanged() ), this, SLOT( movieViewSelectionChanged() ) );
	connect( mMovieView, SIGNAL( contextMenuRequested(const QModelIndex&, QContextMenuEvent::Reason) ), this, SLOT( showMovieContextMenu(const QModelIndex&) ) );
}

/*!
	Shows the preferences dialog.
	\todo maybe dialog pages should better not be added here
*/
void MvdMainWindow::showPreferences()
{
	MvdSettingsDialog sd(this);
	sd.exec();
}

/*!
	Shows the application log.
	\todo Write some sort of log viewer.
*/
void MvdMainWindow::showLog()
{
	QString logPath = paths().logFile();
	logPath = QDir::cleanPath(logPath).prepend("file://");

	QStringList searchPaths;
	searchPaths << "C:/Documents and Settings/blue/Dati applicazioni/BlueSoft/Movida/";

	QTextBrowser* viewer = new QTextBrowser;
	viewer->setAttribute(Qt::WA_DeleteOnClose, true);
	viewer->setSearchPaths(searchPaths);
	viewer->setSource(QUrl("Movida.log"));
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
		if (!closeCollection(true))
		{
			e->ignore();
			return;
		}
	}

	MvdSettings& p = Movida::settings();
	p.setByteArray("main_window_state", saveState(), "gui_appearance");
	p.setBool("start_maximized", isMaximized(), "gui_appearance");
	p.setRect("main_window_rect", frameGeometry(), "gui_appearance");

	p.setStringList("recent_files", mRecentFiles, "gui_general");

	MvdCore::storeStatus();
}

/*!
	Re-opens a recently opened archive.
*/
void MvdMainWindow::openRecentFile(QAction* a)
{
	if (a == 0)
		return;
	
	if (!closeCollection(true))
		return;
	
	QString file = a->data().toString();
	
	if (!loadCollection(file))
	{
		// remove evtl. missing files
		refreshRecentFilesMenu();
	}
}

/*!
	Adds a new item to the MRU files menu.
*/
void MvdMainWindow::addRecentFile(const QString& file)
{
	QFileInfo fi(file);

	// avoid duplicate entries
	for (int i = 0; i < mRecentFiles.size(); ++i)
	{
		if (fi == QFileInfo(mRecentFiles.at(i)))
			return;
	}
	
	mRecentFiles.append(file);	
	refreshRecentFilesMenu();
}

/*!
	Refreshes the MRU files menu removing non existing files.
*/
void MvdMainWindow::refreshRecentFilesMenu()
{
	if (mRecentFiles.isEmpty())
	{
		mMN_FileMRU->setEnabled(false);
		mA_FileOpenLast->setEnabled(false);
		return;
	}
	
	for (int i = 0; i < mRecentFiles.size(); ++i)
	{
		QFile f(mRecentFiles.at(i));
		if (!f.exists())
			mRecentFiles.removeAt(i);
	}
	
	mMN_FileMRU->setEnabled(!mRecentFiles.isEmpty());
	mA_FileOpenLast->setEnabled(!mRecentFiles.isEmpty());
	
	bool ok;
	int max = Movida::settings().getInt("max_recent_files", "gui_general", &ok);
	if (!ok || max < 1)
		max = MVD_DEFAULT_MAX_RECENT_FILES;

	while (mRecentFiles.size() > max)
		mRecentFiles.removeLast();
}

/*!
	Fills the MRU files menu.
*/
void MvdMainWindow::fillRecentFilesMenu()
{
	mMN_FileMRU->clear();
	
	for (int i = 0; i < mRecentFiles.size(); ++i)
	{
		/*! \todo elide long filenames for action text */
		QAction* a = mMN_FileMRU->addAction(QString("&%1 %2").arg(i).arg(mRecentFiles.at(i)));
		a->setData(QVariant(mRecentFiles.at(i)));
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
		txt = QString("%1 - %4").arg(_CAPTION_).arg(tr("New Collection"));
	}
	else
	{
		if (!mCollection->path().isEmpty())
			txt = QString("%1 - %4").arg(_CAPTION_).arg(mCollection->path());
		else txt = QString("%1 - %4").arg(_CAPTION_).arg(mCollection->fileName());
	}

	if (mCollection != 0 && mCollection->isModified())
		txt.append("*");

	setWindowTitle(txt);
}

/*!
	Closes the current collection and initializes a new empty collection.
	Returns false if the collection has not been closed.
	User confirm required if \p confirm is true and the confirm is enabled
	in the application preferences.
*/
bool MvdMainWindow::closeCollection(bool confirm)
{
	if (mCollection == 0)
		return true;
	
	bool isNewEmptyCollection = mCollection->path().isEmpty() && mCollection->isEmpty();

	if (mCollection->isModified() && !isNewEmptyCollection)
	{
		int res = QMessageBox::question(this, _CAPTION_, 
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
	
	if (confirm && Movida::settings().getBool("confirm_close_db", "gui_general"))
	{
		if (QMessageBox::question(this, _CAPTION_, tr("Do you really want to close the collection?"),
			QMessageBox::Yes, QMessageBox::No|QMessageBox::Escape) != QMessageBox::Yes)
			return false;
	}
	
	mMovieModel->setMovieCollection(0);

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
	if (!closeCollection(true))
		return true;

	/*! \todo add thread to load coll. and (_IMPORTANT_) lock gui! */
	bool res = false;
	
	if (mCollection == 0)
		createNewCollection();

	// Avoid the colelctionModified() signal emission.
	mCollection->setModifiedStatus(true);

	res = mCollection->load(file);
	
	mCollection->setModifiedStatus(false);

	if (!res)
	{
		delete mCollection;
		mCollection = 0;

		//! \todo Remove the file from the recent files list?
		QMessageBox::warning(this, _CAPTION_, tr("Failed to load the collection."));

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
	if (!closeCollection(true))
		return false;
	
	MvdSettings& p = Movida::settings();

	QString lastDir = p.getString("last_archive", "directories");

	QString file = QFileDialog::getOpenFileName(
		this, _CAPTION_, 
		lastDir, 
		tr("Movida Movie Collection (*.mmc)")
	);
	
	if (file.isNull())
		return false;
	
	int sep = file.lastIndexOf("/");
	if (sep > 0)
	{
		if (p.getBool("use_last_archive", "directories"))
			p.setString("last_archive", file.left(sep), "directories");
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

	QString lastDir = Movida::settings().getString("last_archive", "directories");

	QString filename = QFileDialog::getSaveFileName(this, _CAPTION_, lastDir, "*.mmc");
	if (filename.isEmpty())
		return false;

	int sep = filename.lastIndexOf("/");
	if (sep > 0)
	{
		if (Movida::settings().getBool("use_last_archive", "directories"))
			Movida::settings().setString("last_archive", filename.left(sep), "directories");
	}

	bool res = mCollection->save(filename);

	if (res)
	{
		mCollection->setModifiedStatus(false);
		mA_FileSave->setEnabled(false);
		updateCaption();
	}

	return res;
}

/*!
	Saves the current collection.
*/
bool MvdMainWindow::saveCollection()
{
	Q_ASSERT(mCollection != 0);
	Q_ASSERT(!mCollection->isEmpty());

	bool res = mCollection->save();
	if (res)
	{
		mCollection->setModifiedStatus(false);
		mA_FileSave->setEnabled(false);
		updateCaption();
	}
	else
	{
		QMessageBox::warning(this, _CAPTION_, tr("Failed to save the collection."));
	}
	return res;
}

/*!
	Re-opens the last closed collection.
*/
void MvdMainWindow::loadLastCollection()
{
	if (mRecentFiles.isEmpty())
		return;
		
	loadCollection(mRecentFiles.at(0));
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
	connect (mCollection, SIGNAL(movieChanged(movieid)), this, SLOT(movieChanged(movieid)) );
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
		

		int catID1 = mCollection->smd().addTag("Terry Gilliam Collection");
		int catID2 = mCollection->smd().addTag("I Love Berry White");
		int catID3 = mCollection->smd().addTag("Loaned to Mike");

		//int idGEN1 = mCollection->smd().addGenre("My Science fiction");
		//int idGEN2 = mCollection->smd().addGenre("My Comedy");

		smdid idPER1 = mCollection->smd().addPerson("CMike", "CThomson");
		smdid idPER2 = mCollection->smd().addPerson("CBob", "CNirvana");
		smdid idPER4 = Movida::globalSD().addPerson("Nicolas", "Cage");
		smdid idPER5 = Movida::globalSD().addPerson("Nicole", "Kidman");

		smdid idURL1 = Movida::globalSD().addUrl("http://www.nicolekidman.com", "Official Nicole Kidman Website");
		smdid idURL2 = Movida::globalSD().addUrl("http://www.imdb.com", "International MvdMovie DB");
		smdid idURL3 = mCollection->smd().addUrl("http://osdab/newsite", "OSDaB X");
		smdid idURL4 = mCollection->smd().addUrl("http://osdab", "");

		smdid idGEN1G = Movida::globalSD().findGenre("Comedy");
		smdid idGEN2G = Movida::globalSD().findGenre("Horror");

		MvdMovie m;
		m.setTitle("Paura E Delirio A Las Vegas");
		m.setOriginalTitle("Fear And Loathing In Las Vegas");
		m.setEdition("Collector's Edition");
		m.setProductionYear("1992");
		m.setReleaseYear("2004");
		m.addCountry(QLocale::Italy);
		m.addCountry(QLocale::UnitedStates);
		m.addLanguage(QLocale::Italian);
		m.addLanguage(QLocale::English);
		m.setRunningTime(112);
		m.setRating(4);
		m.addTag(catID1);
		m.addTag(catID2);
		m.addTag(catID3);
		m.addGenre(idGEN1G);
		m.addGenre(idGEN2G);
		m.addActor(idPER2);
		m.addActor(idPER1, QStringList() << "Ms. Katsuma" << "Hacker" << "Naima");
		m.addActor(idPER5, QStringList() << "The Boss");
		m.addDirector(idPER4);
		m.addUrl(idURL1);
		m.addUrl(idURL2);
		m.addUrl(idURL3);
		m.addUrl(idURL4);
		m.setPlot("QDialogButtonBox now sets the default button to the first button with   the Accept role if no other button has explicitly been set as the default when it is shown. This is to stop a regression where using thec autoDefault property with the Mac and Cleanlooks styles would set the Cancel button as the default.");
		m.setNotes("QDialogButtonBox now sets the default button to the first button with   the Accept role if no other button has explicitly been set as the default when it is shown. This is to stop a regression where using thec autoDefault property with the Mac and Cleanlooks styles would set the Cancel button as the default.");

		movieid id = mCollection->addMovie(m);

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
	mMovieView->selectIndex( mMovieModel->index( mMovieModel->rowCount()-1, 0 ) );
	collectionModified();
}

/*!
	Returns the default window size and position.
*/
QRect MvdMainWindow::defaultWindowRect() const
{
	QDesktopWidget* desktop = QApplication::desktop();
	int screenWidth = desktop->width();
	int screenHeight = desktop->height();

	// set width and height to the window size we want
	int width = (int)(screenWidth * 0.8);
	int height = (int)(screenHeight * 0.8);
	
	int posX = (screenWidth - width) / 2;
	int posY = (screenHeight - height) / 2;
	
	return QRect(posX, posY, width, height);
}

void MvdMainWindow::newCollection()
{
	if (mCollection == 0)
		return;

	if (closeCollection(true))
		mA_FileNew->setDisabled(true);
}

void MvdMainWindow::editPreviousMovie()
{
	QModelIndex prev = mMovieView->indexAbove(mMovieView->selectedIndex());
	editMovie(prev);
}

void MvdMainWindow::editCurrentMovie()
{
	editMovie(mMovieView->selectedIndex());
}

void MvdMainWindow::editNextMovie()
{
	QModelIndex next = mMovieView->indexBelow(mMovieView->selectedIndex());
	editMovie(next);
}

void MvdMainWindow::duplicateCurrentMovie()
{
	movieid id = modelIndexToId(mMovieView->selectedIndex());
	if (id == 0)
		return;

	MvdMovie movie = mCollection->movie(id);
	if (!movie.isValid())
		return;

	bool ok;
	QString text = QInputDialog::getText(this, _CAPTION_,
		tr("Please enter a (unique) title for the new movie:"), QLineEdit::Normal,
		movie.title(), &ok);

	if (!ok || text.isEmpty())
		return;

	//! \todo Validate title for duplicated movie
	if (text.toLower() == movie.title().toLower())
	{
		QMessageBox::warning(this, _CAPTION_, tr("Sorry, but the new title must be unique in this collection."));
		return;
	}

	movie.setTitle(text);
	if (mCollection->addMovie(movie) == 0)
		QMessageBox::warning(this, _CAPTION_, tr("Sorry but the movie could not be duplicated."));
}

void MvdMainWindow::editMovie(const QModelIndex& index)
{
	movieid id = modelIndexToId(index);
	bool resetRequired = false;

	if (mMovieEditor.isNull())
		mMovieEditor = new MvdMovieEditor(mCollection, this);
	else
		resetRequired = true;
	
	mMovieEditor->setAttribute(Qt::WA_DeleteOnClose, true);
	
	if (!mMovieEditor->setMovie(id, true))
		return;
	
	mMovieView->setCurrentIndex(index);

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

	i = mMovieView->indexAbove(index);
	if (i.isValid())
	{
		id = modelIndexToId(i);
		if (id != 0)
		{
			mMovieEditor->setPreviousEnabled(true);
			prev = true;
		}
	}

	i = mMovieView->indexBelow(index);
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

	mMovieEditor->show();
}

quint32 MvdMainWindow::modelIndexToId(const QModelIndex& index) const
{
	QAbstractItemModel* model = mMovieView->model();
	if (model == 0)
		return 0;

	bool ok;
	quint32 id = model->data(index, MvdCollectionModel::MovieIdRole).toUInt(&ok);
	return ok ? id : 0;
}

void MvdMainWindow::removeCurrentMovie()
{
	QModelIndex idx = mMovieView->selectedIndex();
	removeMovie(idx);
}

void MvdMainWindow::removeMovie(const QModelIndex& index)
{
	movieid id = modelIndexToId(index);
	if (id == 0)
		return;

	bool confirm = Movida::settings().getBool("confirm_remove_db", "gui_general");
	if (confirm)
	{
		int res = QMessageBox::question(this, _CAPTION_, tr("Are you sure you want to delete this movie?"),
			QMessageBox::Yes, QMessageBox::No);
		if (res != QMessageBox::Yes)
			return;
	}

	mCollection->removeMovie(id);

	movieViewSelectionChanged();
	collectionModified();
}

void MvdMainWindow::movieViewSelectionChanged()
{
	bool enable = mMovieView->hasSelectedRows();
	mA_CollRemMovie->setEnabled(enable);
	mA_CollEdtMovie->setEnabled(enable);
	mA_CollDupMovie->setEnabled(enable);

	updateDetailsView();
}

void MvdMainWindow::updateDetailsView()
{
	movieid current = modelIndexToId(mMovieView->selectedIndex());
	if (current == 0)
	{
		mDetailsView->clear();
		return;
	}

	MvdMovie movie = mCollection->movie(current);
	QString html = tmanager().movieToHtml(movie, *mCollection, "blue.xsl");
	mDetailsView->setHtml(html);
}

void MvdMainWindow::movieChanged(movieid id)
{
	movieid current = modelIndexToId(mMovieView->selectedIndex());

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
	
	QAbstractItemModel* model = mMovieView->model();
	if (model == 0)
		return;

	bool ok;
	movieid id = model->data(index, MvdCollectionModel::MovieIdRole).toUInt(&ok);
	if (!ok || id == 0)
		return;

	MvdMovie movie = mCollection->movie(id);
	if (!movie.isValid())
		return;

	QMenu menu;
}




//! \todo DEBUG
void MvdMainWindow::testSlot()
{
#if 1
	MvdSmartView* view = new MvdSmartView(mMovieModel);
	view->setAttribute(Qt::WA_DeleteOnClose, true);
	
	QList<int> cols;
	cols << 0 << 1 << 2 << 3 << 4;	
	view->setMainModelColumn(0);
	view->setAdditionalModelColumns(cols);

	view->show();
#endif
}
