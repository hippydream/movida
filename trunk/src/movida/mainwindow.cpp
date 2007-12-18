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

#include "collectionloader.h"
#include "collectionmetaeditor.h"
#include "collectionmodel.h"
#include "collectionsaver.h"
#include "core.h"
#include "dockwidget.h"
#include "filterproxymodel.h"
#include "filterwidget.h"
#include "guiglobal.h"
#include "logger.h"
#include "mainwindow.h"
#include "movie.h"
#include "moviecollection.h"
#include "pathresolver.h"
#include "plugininterface.h"
#include "rowselectionmodel.h"
#include "settings.h"
#include "settingsdialog.h"
#include "smartview.h"
#include "templatemanager.h"
#include "treeview.h"
#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLibrary>
#include <QList>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextBrowser>
#include <QTimer>
#include <QUrl>
#include <QtDebug>

using namespace Movida;
MvdMainWindow* Movida::MainWindow = 0;

/*!
	\class MvdMainWindow mainwindow.h
	\ingroup Movida

	\brief The Movida main window.
*/


/*!
	Creates a new main window and initializes the application.
*/
MvdMainWindow::MvdMainWindow(QWidget* parent)
: QMainWindow(parent), mAG_SortActions(0), mA_SortDescending(0),
mMB_MenuBar(menuBar()), mCollection(0), mMovieEditor(0)
{
	Q_ASSERT_X(!Movida::MainWindow, "MvdMainWindow", "Internal Error. Only a single instance of MvdMainWindow can be created.");
	Movida::MainWindow = this;

	MVD_WINDOW_ICON
	setWindowTitle("Movida - The free movie collection manager");

	mMovieModel = new MvdCollectionModel(this);

	setupUi();
	setupConnections();
	
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
	p.setDefaultValue("movida/use-history", true);
	p.setDefaultValue("movida/quick-filter/case-sensitive", false);
	p.setDefaultValue("movida/quick-filter/attributes", mFilterModel->quickFilterAttributes());

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

	mFilterWidget->setCaseSensitivity(p.value("movida/quick-filter/case-sensitive").toBool() ? Qt::CaseSensitive : Qt::CaseInsensitive);
	mFilterModel->setQuickFilterAttributes(p.value("movida/quick-filter/attributes").toByteArray());
	
	// a new empty collection is always open at startup
	mA_FileNew->setDisabled(true);
	mA_FileSave->setDisabled(true);
	mA_FileSaveAs->setDisabled(true);

	movieViewSelectionChanged();
	mFilterModel->sortByAttribute(Movida::TitleAttribute, Qt::AscendingOrder);
	currentViewChanged();

	createNewCollection();
	loadPlugins();

	statusBar()->showMessage(tr("Movida is ready!"));
	iLog() << "Movida is ready!";
}

/*!
	\todo Review evtl. needed deletes
*/
MvdMainWindow::~MvdMainWindow()
{
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
	iLog() << "Movida is closing...";

	if (mCollection && mCollection->isModified())
	{
		if (!closeCollection())
		{
			e->ignore();
			return;
		}
	}
}

//! This method is called before closing the app or on a crash.
void MvdMainWindow::cleanUp()
{
	MvdSettings& p = Movida::settings();
	p.setValue("movida/appearance/main-window-state", saveState());
	p.setValue("movida/appearance/start-maximized", isMaximized());
	p.setValue("movida/appearance/main-window-size", size());
	p.setValue("movida/appearance/main-window-pos", pos());
	p.setValue("movida/quick-filter/case-sensitive", mFilterWidget->caseSensitivity() == Qt::CaseSensitive);
	MvdCore::storeStatus();

	//! \todo Clean up plugins

	Movida::paths().removeDirectoryTree(Movida::paths().tempDir());

	if (mCollection && mCollection->isModified()) {
		//! \todo Emergency save!
	}
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
	
	Movida::settings().setValue("movida/recent-files", list);

	mA_FileOpenLast->setDisabled(list.isEmpty());
}

/*!
	Checks and updates the status of the File submenu (e.g. removing non existing
	recent files and disabling empty menus).
*/
void MvdMainWindow::updateFileMenu()
{
	// RECENT FILES

	QStringList list = Movida::settings().value("movida/recent-files").toStringList();
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

	// IMPORT
	mMN_FileImport->setDisabled(mMN_FileImport->isEmpty());
}

//! Populates the sort menu according to the current view.
void MvdMainWindow::updateViewSortMenu()
{
	mMN_ViewSort->clear();
	delete mAG_SortActions;

	QList<Movida::MovieAttribute> atts;
	 
	if (mMainViewStack->currentWidget() == mSmartView) {
		atts.append(Movida::TitleAttribute);
		atts.append(Movida::ReleaseYearAttribute);
		atts.append(Movida::ProductionYearAttribute);
	} else {
		atts = Movida::movieAttributes();
	}
	
	Movida::MovieAttribute currentAttribute = mFilterModel->sortAttribute();

	mAG_SortActions = new QActionGroup(this);
	mAG_SortActions->setExclusive(true);
	connect(mAG_SortActions, SIGNAL(triggered(QAction*)), this, SLOT(sortActionTriggered(QAction*)));

	for (int i = 0; i < atts.size(); ++i) {
		Movida::MovieAttribute a = atts.at(i);
		QAction* action = mAG_SortActions->addAction(Movida::movieAttributeString(a));
		action->setCheckable(true);
		if (a == currentAttribute)
			action->setChecked(true);
		action->setData((int)a);
	}

	mMN_ViewSort->addActions(mAG_SortActions->actions());
	
	mMN_ViewSort->addSeparator();
	
	if (!mA_SortDescending) {
		mA_SortDescending = new QAction(tr("&Descending"), this);
		mA_SortDescending->setCheckable(true);
		connect(mA_SortDescending, SIGNAL(triggered()), this, SLOT(sortActionTriggered()));
	}

	mMN_ViewSort->addAction(mA_SortDescending);
}

/*!
	Updates the main window title.
*/
void MvdMainWindow::updateCaption()
{
	/*! \todo setting: show full path on titlebar */

	QString name;
	if (mCollection)
		name = mCollection->metaData(MvdMovieCollection::NameInfo);
	
	if (mCollection && !name.isEmpty()) {
		;
	} else if (!mCollection || mCollection->fileName().isEmpty()) {
		name = tr("New Collection");
	} else {
		if (!mCollection->path().isEmpty())
			name = mCollection->path();
		else name = mCollection->fileName();
	}

	name = QString("%1 - %2").arg(MVD_CAPTION).arg(name);

	if (mCollection && mCollection->isModified())
		name.append("*");

	setWindowTitle(name);
}

/*!
	Closes the current collection and initializes a new empty collection.
	Returns false if the collection has not been closed.
*/
bool MvdMainWindow::closeCollection()
{
	resetFilter();

	if (!mCollection)
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

	// Ensure plugins have a valid collection to work on
	createNewCollection();
	
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
	
	if (!mCollection)
		createNewCollection();

	// Avoid the colelctionModified() signal emission.
	mCollection->setModifiedStatus(true);

	MvdCollectionLoader::StatusCode res = MvdCollectionLoader::load(mCollection, file);
	mCollection->setModifiedStatus(false);

	if (res != MvdCollectionLoader::NoError) {
		createNewCollection();
		QMessageBox::warning(this, MVD_CAPTION, tr("Failed to load the collection."));
		return false;
	}
	
	// Update GUI controls
	collectionModified();

	Movida::MovieAttribute a = Movida::TitleAttribute;
	QAction* ca = mAG_SortActions->checkedAction();
	if (ca)
		a = (Movida::MovieAttribute)ca->data().toInt();
	
	mFilterModel->sortByAttribute(a, mFilterModel->sortOrder());

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
	Q_ASSERT(mCollection);
	Q_ASSERT(!mCollection->isEmpty());

	QString lastDir = Movida::settings().value("movida/directories/last-collection").toString();

	QString filename = QFileDialog::getSaveFileName(this, MVD_CAPTION, lastDir, "*.mmc");
	if (filename.isEmpty())
		return false;

	if (!filename.endsWith(".mmc"))
		filename.append(".mmc");

	addRecentFile(filename);

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
	Q_ASSERT(mCollection);
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
	if (mCollection) {
		delete mCollection;
	}

	mCollection = new MvdMovieCollection();
	MvdCore::pluginContext()->collection = mCollection;
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
	if (!mCollection)
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
	if (!mCollection)
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
	QString html = tmanager().movieToHtml(movie, *mCollection, "Blue");
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
	if (sender() == mSmartView && mAG_SortActions)
	{
		QMenu* sortMenu = menu.addMenu(tr("Sort"));
		sortMenu->addActions(mAG_SortActions->actions());
		sortMenu->addSeparator();
		sortMenu->addAction(mA_SortDescending);
	}
	
	QAction* res = menu.exec(QCursor::pos());

	Qt::SortOrder so = mFilterModel->sortOrder() == Qt::AscendingOrder ? 
		Qt::DescendingOrder : Qt::AscendingOrder;

	Q_UNUSED(res);
	Q_UNUSED(so);
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

	QString path = paths().resourcesDir(Movida::UserScope).append("Plugins");
	loadPluginsFromDir(path);
	path = paths().resourcesDir(Movida::SystemScope).append("Plugins");
	if (!path.isEmpty())
		loadPluginsFromDir(path);
}

//! \internal
void MvdMainWindow::loadPluginsFromDir(const QString& path)
{
	if (!mCollection)
		createNewCollection();

	QDir pluginDir(path);

#if defined(Q_WS_WIN)
	QString ext = "*.dll";
	QString prefix = "mpi";
#elif defined(Q_WS_MAC)
	QString ext = "*.dylib";
	QString prefix = "libmpi";
#else
	QString ext = "*.so";
	QString prefix = "libmpi";
#endif

	QFileInfoList list = pluginDir.entryInfoList(QStringList() << ext);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo& fi = list[i];
		QString name = fi.completeBaseName();

		if (!name.startsWith(prefix))
			continue;

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

		QString dataStorePath = paths().resourcesDir(Movida::UserScope).append("Plugins/").append(fi.completeBaseName());
		if (!QFile::exists(dataStorePath))
		{
			QDir d;
			if (!d.mkpath(dataStorePath))
			{
				eLog() << "Failed to create user data store for plugin: " << dataStorePath;
				continue;
			}
		}

		dataStorePath = MvdCore::toLocalFilePath(dataStorePath, true);
		iface->setDataStore(dataStorePath, Movida::UserScope);
		iLog() << QString("'%1' plugin user data store: ").arg(info.name).append(dataStorePath);

		// Create global data store
		dataStorePath = paths().resourcesDir(Movida::SystemScope);
		if (!dataStorePath.isEmpty())
		{
			dataStorePath.append("Plugins/").append(fi.completeBaseName());
			bool ok = true;
			if (!QFile::exists(dataStorePath))
			{
				QDir d;
				if (!d.mkpath(dataStorePath))
				{
					wLog() << "Failed to create global data store for plugin: " << dataStorePath;
					ok = false;
				}
			} else ok = true;

			if (ok)
			{
				dataStorePath = MvdCore::toLocalFilePath(dataStorePath, true);
				iface->setDataStore(dataStorePath, Movida::SystemScope);
				iLog() << QString("'%1' plugin system data store: ").arg(info.name).append(dataStorePath);
			}
		}

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

			if (a.type.testFlag(MvdPluginInterface::ImportAction)) {
				mMN_FileImport->addAction(qa);
			}
		}

		connect( signalMapper, SIGNAL(mapped(const QString&)), 
			iface, SLOT(actionTriggered(const QString&)) );
	}
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

//! Returns the current collection. This method will create a new collection if none exists.
MvdMovieCollection* MvdMainWindow::currentCollection()
{
	if (!mCollection)
		createNewCollection();
	Q_ASSERT(mCollection);
	return mCollection;
}

//! Shows the collection metadata editor.
void MvdMainWindow::showMetaEditor()
{
	//! Handle read-only collections
	MvdCollectionMetaEditor editor(this);
	editor.setCollection(currentCollection());
	editor.exec();
}

//! Updates gui elements after the current view has changed.
void MvdMainWindow::currentViewChanged()
{
	updateViewSortMenu();
}

void MvdMainWindow::sortActionTriggered(QAction* a)
{
	if (!a)
		a = mAG_SortActions->checkedAction();

	Qt::SortOrder so = mA_SortDescending && mA_SortDescending->isChecked() ? Qt::DescendingOrder : Qt::AscendingOrder;
	mFilterModel->sortByAttribute(a ? (Movida::MovieAttribute)a->data().toInt() : Movida::TitleAttribute, so);
}

//! Updates the sort actions whenever the tree view is sorted by the user
void MvdMainWindow::treeViewSorted(int)
{
	if (!mAG_SortActions)
		return;

	Movida::MovieAttribute attrib = mFilterModel->sortAttribute();
	QList<QAction*> alist = mAG_SortActions->actions();
	for (int i = 0; i < alist.size(); ++i) {
		QAction* a = alist.at(i);
		if ((Movida::MovieAttribute)a->data().toInt() == attrib) {
			a->setChecked(true);
			break;
		}
	}

	if (mA_SortDescending)
		mA_SortDescending->setChecked(mFilterModel->sortOrder() == Qt::DescendingOrder);
}

//! Updates GUI elements after the collection model has been sorted.
void MvdMainWindow::collectionModelSorted()
{
	mTreeView->header()->setSortIndicator((int)mFilterModel->sortAttribute(), mFilterModel->sortOrder());
}

void MvdMainWindow::keyPressEvent(QKeyEvent* e)
{
	int key = e->key();
	QString ttf = mFilterWidget->editor()->text();
	QString text = e->text();

	if (mFilterWidget->isVisible()) {
		switch (key) {
		case Qt::Key_Escape:
			resetFilter();
			return;
		case Qt::Key_Backspace:
			ttf.chop(1);
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			// Return/Enter key events are not accepted by QLineEdit
			return;
		default:
			if (text.isEmpty()) {
				QMainWindow::keyPressEvent(e);
				return;
			}
			ttf += text;
		}
	} else {
		if (text.isEmpty() || text[0].isSpace() || !text[0].isPrint()) {
			QMainWindow::keyPressEvent(e);
			return;
		}
		if (text.startsWith(QLatin1Char('/'))) {
			mFilterWidget->editor()->clear();
			showFilterWidget();
			return;
		}
		ttf = text;
		mFilterWidget->show();
		mFilterWidget->setNoResultsWarningVisible(false);
	}

	mFilterWidget->editor()->setText(ttf);
	filter(ttf);
}

bool MvdMainWindow::eventFilter(QObject* o, QEvent* e)
{
	if (o == mFilterWidget->editor()) {
		if (e->type() == QEvent::FocusIn && mHideFilterTimer->isActive())
			mHideFilterTimer->stop();
	}

	return QMainWindow::eventFilter(o, e);
}

void MvdMainWindow::showFilterWidget()
{
	mFilterWidget->show();
	mFilterWidget->setNoResultsWarningVisible(false);
	mFilterWidget->editor()->setFocus(Qt::ShortcutFocusReason);
	mFilterWidget->editor()->selectAll();
	mHideFilterTimer->stop();
}

//! Applies a filter with the current contents of the filter widget.
void MvdMainWindow::applyCurrentFilter()
{
	filter(mFilterWidget->editor()->text());
	mHideFilterTimer->stop();
}

void MvdMainWindow::filter(QString s)
{
	s = s.trimmed();

	QPalette p = mFilterWidget->editor()->palette();
	p.setColor(QPalette::Active, QPalette::Base, Qt::white);
	
	// PERFORM FILTER
	bool nothingToFilter = true;
	bool hasText = !mFilterWidget->editor()->text().trimmed().isEmpty();
	Qt::CaseSensitivity cs = mFilterWidget->caseSensitivity();
	
	mFilterModel->setFilterCaseSensitivity(cs);
	mFilterModel->setFilterFixedString(s);

	nothingToFilter = mFilterModel->rowCount() == 0;
	
	if (nothingToFilter && hasText)
		p.setColor(QPalette::Active, QPalette::Base, QColor(255, 102, 102));
	
	if (!mFilterWidget->isVisible())
		mFilterWidget->show();

	mFilterWidget->setNoResultsWarningVisible(nothingToFilter && hasText);

	mFilterWidget->editor()->setPalette(p);
	if (!mFilterWidget->editor()->hasFocus() && !hasText)
		mHideFilterTimer->start();
}

void MvdMainWindow::resetFilter()
{
	mFilterModel->setFilterRegExp(QString());
	mFilterWidget->hide();
}

bool MvdMainWindow::isQuickFilterVisible() const
{
	return mFilterWidget->isVisible();
}
