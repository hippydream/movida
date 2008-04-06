/**************************************************************************
** Filename: mainwindow.cpp
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

#include "collectionmetaeditor.h"
#include "collectionmodel.h"
#include "dockwidget.h"
#include "filterproxymodel.h"
#include "filterwidget.h"
#include "guiglobal.h"
#include "infopanel.h"
#include "mainwindow.h"
#include "moviemasseditor.h"
#include "movietreeview.h"
#include "rowselectionmodel.h"
#include "settingsdialog.h"
#include "shareddataeditor.h"
#include "shareddatamodel.h"
#include "smartview.h"
#include "mvdcore/collectionloader.h"
#include "mvdcore/collectionsaver.h"
#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/pathresolver.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"
#include "mvdcore/templatemanager.h"
#include <QActionGroup>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHttp>
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
#include <QTemporaryFile>
#include <QTextBrowser>
#include <QTimer>
#include <QToolBar>
#include <QtDebug>

using namespace Movida;
MvdMainWindow* Movida::MainWindow = 0;

Q_DECLARE_METATYPE(MvdCollectionLoader::Info);


/*!
	\class MvdMainWindow mainwindow.h
	\ingroup Movida

	\brief The Movida main window.
*/

//! \todo Option to always exclude loaned movies from searches

/*!
	Creates a new main window and initializes the application.
*/
MvdMainWindow::MvdMainWindow(QWidget* parent)
: QMainWindow(parent), 
mInfoPanelClosedByUser(false), mCollection(0), mMovieEditor(0), mHttp(0), 
mDraggingSharedData(false), mSavedFilterMessage(0)
{
	Q_ASSERT_X(!Movida::MainWindow, "MvdMainWindow", "Internal Error. Only a single instance of MvdMainWindow can be created.");
	Movida::MainWindow = this;

	MVD_WINDOW_ICON
	setWindowTitle("Movida - The free movie collection manager");

	mMovieModel = new MvdCollectionModel(this);
	mSharedDataModel = new MvdSharedDataModel(Movida::PersonRole, this);

	setupUi();
	setupConnections();
#ifdef MVD_DEBUG_TOOLS
	setupDebugTools();
#endif
	
	// Set some GUI related constants
	QHash<QString,QVariant> parameters;
	parameters.insert("movida/maximum-recent-files", 10);
	parameters.insert("movida/default-recent-files", 5);
	parameters.insert("movida/max-menu-items", 10);
	parameters.insert("movida/message-timeout-ms", 5 * 1000);
	parameters.insert("movida/poster-default-width", 70);
	parameters.insert("movida/poster-aspect-ratio", qreal(0.7));
	parameters.insert("movida/imdb-movie-url", "http://akas.imdb.com/title/tt%1");
	parameters.insert("movida/mime/movie", "application/movida-movie");
	parameters.insert("movida/mime/movie-attributes", "application/movida-movie-attributes");
	parameters.insert("movida/mime/movie-filter", "application/movida-movie-filter");
	parameters.insert("movida/d&d/max-pixmaps", 5);
	parameters.insert("movida/d&d/max-values", 8);
	MvdCore::registerParameters(parameters);

	MvdCore::loadStatus();
	MvdSettings& p = Movida::settings();

	// Set default settings
	p.setDefaultValue("movida/maximum-recent-files", 5);
	p.setDefaultValue("movida/confirm-delete-movie", true);
	p.setDefaultValue("movida/directories/use-last-collection", true);
	p.setDefaultValue("movida/movie-list/initials", false);
	p.setDefaultValue("movida/use-history", true);
	p.setDefaultValue("movida/max-history-items", 20);
	p.setDefaultValue("movida/quick-filter/case-sensitive", false);
	p.setDefaultValue("movida/quick-filter/attributes", mFilterModel->quickFilterAttributes());
	p.setDefaultValue("movida/view-mode", "smart");
	p.setDefaultValue("movida/smart-view/item-size", "medium");

	// Initialize core library && load user settings
	QStringList recentFiles = p.value("movida/recent-files").toStringList();
	int max = p.value("movida/maximum-recent-files").toInt();
	int recentFilesCount = recentFiles.size();
	while (recentFiles.size() > max)
		recentFiles.removeLast();
	if (recentFilesCount != recentFiles.size())
		p.setValue("movida/recent-files", recentFiles);

	mA_FileOpenLast->setDisabled(recentFiles.isEmpty());

	QString viewMode = p.value("movida/view-mode").toString();
	if (viewMode == "smart")
		mA_ViewModeSmart->activate(QAction::Trigger);
	else 
		mA_ViewModeTree->activate(QAction::Trigger);

	viewMode = p.value("movida/smart-view/item-size").toString();
	if (viewMode == "small")
		zoomOut();
	else if (viewMode == "large")
		zoomIn();

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
	mTreeView->sortByColumn((int) Movida::TitleAttribute, Qt::AscendingOrder);
	mSharedDataModel->sort(0, Qt::AscendingOrder);
	currentViewChanged();

	setAcceptDrops(true);

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
	Stores preferences and application status.
*/
void MvdMainWindow::closeEvent(QCloseEvent* e)
{
	iLog() << "Movida is closing...";

	if (mCollection && mCollection->isModified()) {
		if (!closeCollection()) {
			e->ignore();
			return;
		}
	}
	
	cleanUp();
}

//! This method is called before closing the app or on a crash.
void MvdMainWindow::cleanUp()
{
	MvdSettings& p = Movida::settings();
	p.setValue("movida/appearance/main-window-state", saveState());
	p.setValue("movida/appearance/start-maximized", isMaximized());
	if (!isMaximized()) {
		p.setValue("movida/appearance/main-window-size", size());
		p.setValue("movida/appearance/main-window-pos", pos());
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

	Movida::paths().removeDirectoryTree(Movida::paths().tempDir());

	if (mCollection && mCollection->isModified()) {
		//! \todo Emergency save!
	}

	// This must be called last!!! Singletons should not be accessed from now on!
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
	loadCollection(file);

	// Move file to the top of the MRU list
	QStringList list = Movida::settings().value("movida/recent-files").toStringList();
	int idx = list.indexOf(file);
	if (idx < 0)
		return;
	list.removeAt(idx);
	list.prepend(file);
	Movida::settings().setValue("movida/recent-files", list);
}

/*!
	Adds a new item to the MRU files menu.
*/
void MvdMainWindow::addRecentFile(const QString& file)
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
	foreach (QAction* a, mAG_ViewSort->actions())
		mAG_ViewSort->removeAction(a);

	QList<Movida::MovieAttribute> atts;
	QWidget* cw = mMainViewStack->currentWidget();

	if (cw == mSmartView) {
		atts = Movida::movieAttributes(Movida::SmartViewAttributeFilter);
	} else if (cw == mTreeView) {
		atts = Movida::movieAttributes();
	}
	
	Movida::MovieAttribute currentAttribute = mFilterModel->sortAttribute();

	for (int i = 0; i < atts.size(); ++i) {
		Movida::MovieAttribute a = atts.at(i);
		QAction* action = mAG_ViewSort->addAction(Movida::movieAttributeString(a));
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
	Updates the main window title.
*/
void MvdMainWindow::updateCaption()
{
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

	int res = QMessageBox::Discard;
	
	bool isNewEmptyCollection = mCollection->path().isEmpty() && mCollection->isEmpty();

	if (mCollection->isModified() && !isNewEmptyCollection) {
		res = QMessageBox::question(this, MVD_CAPTION, 
			tr("The collection has been modified.\nDo you want to save the changes?"),
			QMessageBox::Save, QMessageBox::Discard, QMessageBox::Cancel);
	}

	if (res == QMessageBox::Cancel)
		return false;

	if (res == QMessageBox::Save) {
		bool result;
		if (mCollection->path().isEmpty())
			result = saveCollectionDlg();
		else result = saveCollection();

		if (!result)
			return false;
	}
	
	mA_FileNew->setDisabled(true);
	mA_FileSave->setDisabled(true);
	mA_FileSaveAs->setDisabled(true);
	
	updateCaption();
	movieViewSelectionChanged();

	mCollection->clearPersistentData();
	createNewCollection();

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

	this->setEnabled(false);

	MvdCollectionLoader loader;
	loader.setProgressHandler(this, "collectionLoaderCallback");
	MvdCollectionLoader::StatusCode res = loader.load(mCollection, file);
	mCollection->setModifiedStatus(false);

	if (res != MvdCollectionLoader::NoError) {
		createNewCollection();
		QMessageBox::warning(this, MVD_CAPTION, tr("Failed to load the collection."));
	}

	this->setEnabled(true);

	if (res != MvdCollectionLoader::NoError)
		return false;
	
	// Update GUI controls
	collectionModified();

	Movida::MovieAttribute a = Movida::TitleAttribute;
	QAction* ca = mAG_ViewSort->checkedAction();
	if (ca)
		a = (Movida::MovieAttribute)ca->data().toInt();
	
	mTreeView->sortByColumn((int) a, mFilterModel->sortOrder());
	mSharedDataModel->sort(0, mSharedDataModel->sortOrder());

	mInfoPanel->showTemporaryMessage(tr("%1 movies loaded.").arg(mCollection->count()));

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
	mSharedDataModel->setMovieCollection(mCollection);

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
	MvdMovieEditor pd(mCollection, this);
	pd.exec();

	QModelIndex index = movieIdToIndex(pd.movieId());

	// Select the new movie
	if (index.isValid()) {
		QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect;
		mSelectionModel->select(index, flags);
	}
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

void MvdMainWindow::editSelectedMovies()
{
	QModelIndexList list = mTreeView->selectedRows();
	if (list.isEmpty())
		return;
	if (list.size() == 1)
		editMovie(list.at(0));
}

void MvdMainWindow::massEditSelectedMovies()
{
	QModelIndexList list = mTreeView->selectedRows();
	if (list.size() > 1) {
		QList<mvdid> ids;
		for (int i = 0; i < list.size(); ++i)
			ids << movieIndexToId(list.at(i));
		MvdMovieMassEditor med(mCollection, this);
		med.setMovies(ids);
		med.exec();
	}
}

void MvdMainWindow::editNextMovie()
{
	QModelIndex next = mTreeView->indexBelow(mTreeView->selectedIndex());
	editMovie(next);
}

void MvdMainWindow::duplicateCurrentMovie()
{
	mvdid id = movieIndexToId(mTreeView->selectedIndex());
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
	mvdid id = movieIndexToId(index);
	bool resetRequired = false;
	bool exec;

	if (exec = mMovieEditor.isNull()) {
		mMovieEditor = new MvdMovieEditor(mCollection, this);
		mMovieEditor->setAttribute(Qt::WA_DeleteOnClose, true);
	}
	else
		resetRequired = true;
	
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
		id = movieIndexToId(i);
		if (id != 0)
		{
			mMovieEditor->setPreviousEnabled(true);
			prev = true;
		}
	}

	i = mTreeView->indexBelow(index);
	if (i.isValid())
	{
		id = movieIndexToId(i);
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

	if (exec)
		mMovieEditor->exec();
}

mvdid MvdMainWindow::movieIndexToId(const QModelIndex& index) const
{
	bool ok;
	mvdid id = mFilterModel->data(index, Movida::IdRole).toUInt(&ok);
	return ok ? id : MvdNull;
}

QModelIndex MvdMainWindow::movieIdToIndex(mvdid id) const
{
	QModelIndex index = mMovieModel->findMovie(id);
	return index.isValid() ? mFilterModel->mapFromSource(index) : QModelIndex();
}

void MvdMainWindow::removeSelectedMovies()
{
	QModelIndexList list = mTreeView->selectedRows();
	if (list.isEmpty())
		return;

	removeMovies(list);
}

void MvdMainWindow::removeMovie(const QModelIndex& index)
{
	mvdid id = movieIndexToId(index);
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

	QList<mvdid> ids;
	for (int i = 0; i < list.size(); ++i) {
		mvdid id = movieIndexToId(list.at(i));
		if (id != MvdNull) ids.append(id);
	}

	for (int i = 0; i < ids.size(); ++i) {
		mCollection->removeMovie(ids.at(i));
	}

	movieViewSelectionChanged();
	collectionModified();
}

void MvdMainWindow::movieViewSelectionChanged()
{
	QModelIndexList list = mTreeView->selectedRows();

	mA_CollRemMovie->setEnabled(!list.isEmpty());
	mA_CollEdtMovie->setEnabled(list.size() == 1);
	mA_CollMedMovie->setEnabled(list.size() > 1);
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

	mvdid current = movieIndexToId(list.at(0));
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
	mvdid current = movieIndexToId(mTreeView->selectedIndex());

	if (current == 0)
		return;

	if (current == id)
		updateDetailsView();
}

void MvdMainWindow::showMovieContextMenu(const QModelIndex& index)
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (!senderWidget)
		return;

	QMenu menu;
	QAction* addNew = 0;
	QAction* editCurrent = 0;
	QAction* deleteCurrent = 0;
	QAction* deleteSelected = 0;

	bool currentIsSelected = false;

	addNew = menu.addAction(tr("New movie..."));
	if (!mMN_FileImport->isEmpty()) {
		menu.addMenu(mMN_FileImport);
	}

	QModelIndexList selected = mSelectionModel->selectedRows();
	mvdid currentId = movieIndexToId(index);
			bool movieMenuAdded = false;
	if (currentId != MvdNull) {
	
		MvdMovie movie = mCollection->movie(currentId);
		if (movie.isValid()) {
			if (!menu.isEmpty())
				menu.addSeparator();
			movieMenuAdded = true;
			QString title = fontMetrics().elidedText(movie.validTitle(), Qt::ElideMiddle, 300);
			editCurrent = menu.addAction(tr("Edit \"%1\"", "Edit movie").arg(title));
			deleteCurrent = menu.addAction(tr("Delete \"%1\"", "Delete movie").arg(title));
		}
	}

	
	if (selected.size() > 1) {
		if (!menu.isEmpty())
			menu.addSeparator();
		deleteSelected = menu.addAction(tr("Delete selected movies"));
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
					QString title = fontMetrics().elidedText(movie.validTitle(), Qt::ElideMiddle, 300);
					editCurrent = menu.addAction(tr("Edit \"%1\"", "Edit movie").arg(title));
					deleteCurrent = menu.addAction(tr("Delete \"%1\"", "Delete movie").arg(title));
				}
			}
		}
	}

	if (sender() == mSmartView && mAG_ViewSort&& mMovieModel->rowCount()) {
		if (!menu.isEmpty())
			menu.addSeparator();
		QMenu* sortMenu = menu.addMenu(tr("Sort"));
		sortMenu->addActions(mAG_ViewSort->actions());
		sortMenu->addSeparator();
		sortMenu->addAction(mA_ViewSortDescending);
	}
	
	QAction* res = menu.exec(QCursor::pos());
	if (!res)
		return;

	if (res == addNew) {
		addMovie();
	} else if (res == editCurrent) {
		editMovie(currentIsSelected ? selected.first() : index);
	} else if (res == deleteCurrent) {
		removeMovie(currentIsSelected ? selected.first() : index);
	} else if (res == deleteSelected) {
		removeMovies(selected);
	}
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

//! \internal Unloads plugins and frees used memory. Also clears the plugins menu.
void MvdMainWindow::unloadPlugins()
{
	if (mPlugins.isEmpty())
		return;

	while (!mPlugins.isEmpty()) {
		MvdPluginInterface* p = mPlugins.takeFirst();
		iLog() << QLatin1String("Unloading plugin: ") << p->info().name;
		p->unload();
		delete p;
	}

	mMN_Plugins->clear();
}

//! \internal
void MvdMainWindow::loadPlugins()
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
		if (info.uniqueId.trimmed().isEmpty()) {
			wLog() << "Discarding plugin with no unique ID.";
			continue;
		}

		info.uniqueId.replace(QLatin1Char('/'), QLatin1Char('.'));

		bool discard = false;
		foreach (MvdPluginInterface* plug, mPlugins) {
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

		iLog() << QString("'%1' plugin loaded.").arg(info.uniqueId);

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

		for (int j = 0; j < actions.size(); ++j) {
			const MvdPluginInterface::PluginAction& a = actions.at(j);
			QAction* qa = createAction();
			qa->setText(a.text);
			qa->setStatusTip(a.helpText);
			qa->setIcon(a.icon);
			qa->setData(QString("%1/%2").arg(info.uniqueId.trimmed()).arg(a.name));

			connect( qa, SIGNAL(triggered()), this, SLOT(pluginActionTriggered()) );

			pluginMenu->addAction(qa);

			if (a.type.testFlag(MvdPluginInterface::ImportAction)) {
				mMN_FileImport->addAction(qa);
			}
		}

		mPlugins.append(iface);
	}
}

void MvdMainWindow::movieViewToggled(QAction* a)
{
	if (a == mA_ViewModeSmart) {
		mMainViewStack->setCurrentWidget(mSmartView);
		mA_ViewModeZoom->setEnabled(true);
	} else if (a == mA_ViewModeTree) {
		mMainViewStack->setCurrentWidget(mTreeView);
		mA_ViewModeZoom->setEnabled(false);
	}
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
		a = mAG_ViewSort->checkedAction();

	Qt::SortOrder so = mA_ViewSortDescending && mA_ViewSortDescending->isChecked() ? Qt::DescendingOrder : Qt::AscendingOrder;
	mTreeView->sortByColumn(a ? a->data().toInt() : (int) Movida::TitleAttribute, so);
}

//! Updates the sort actions whenever the tree view is sorted by the user
void MvdMainWindow::treeViewSorted(int)
{
	if (!mAG_ViewSort)
		return;

	Movida::MovieAttribute attrib = mFilterModel->sortAttribute();
	QList<QAction*> alist = mAG_ViewSort->actions();
	for (int i = 0; i < alist.size(); ++i) {
		QAction* a = alist.at(i);
		if ((Movida::MovieAttribute)a->data().toInt() == attrib) {
			a->setChecked(true);
			break;
		}
	}

	if (mA_ViewSortDescending)
		mA_ViewSortDescending->setChecked(mFilterModel->sortOrder() == Qt::DescendingOrder);
}

void MvdMainWindow::keyPressEvent(QKeyEvent* e)
{
	int key = e->key();
	QString ttf = mFilterWidget->editor()->text();
	QString text = e->text();

	bool hasModifier = !(e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ALT || e->modifiers().testFlag(Qt::ShiftModifier));
	if (hasModifier|| !shouldShowQuickFilter()) {
		QMainWindow::keyPressEvent(e);
		return;
	}

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
			return;
		}
		ttf = text;
	}

	mFilterWidget->editor()->setText(ttf);
}

bool MvdMainWindow::eventFilter(QObject* o, QEvent* e)
{
	if (o == mFilterWidget->editor()) {
		if (e->type() == QEvent::FocusIn && mHideFilterTimer->isActive())
			mHideFilterTimer->stop();
	}

	else if (o == mSmartView->viewport() || o == mTreeView->viewport()) {
		if (e->type() == QEvent::DragEnter && !mDraggingSharedData) {
			QDragEnterEvent* _e = static_cast<QDragEnterEvent*>(e);
			if (_e->source() == mSharedDataEditor->view()) {
				// Dirty dirty dirty trick to detect the end of a drag operation.
				// Any attempt to use DragLeave or Drop failed.
				connect(_e->mimeData(), SIGNAL(destroyed()), SLOT(sdeDragEnded()));
				sdeDragStarted();
			}
		}
	}

	return QMainWindow::eventFilter(o, e);
}

void MvdMainWindow::showFilterWidget()
{
	mFilterWidget->show();
	mFilterWidget->setMessage(MvdFilterWidget::NoMessage);
	mFilterWidget->editor()->setFocus(Qt::ShortcutFocusReason);
	mFilterWidget->editor()->selectAll();
	mHideFilterTimer->stop();
}

//! Reapplies the current filter to the movie view. Use this when some option changed (e.g. case sensitivity).
void MvdMainWindow::filter()
{
	filter(mFilterWidget->editor()->text());
}

void MvdMainWindow::filter(QString s)
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
		mInfoPanel->setText(tr("Movie filter is active. Clear or close the filter bar to show all movies."));
		mInfoPanel->show();
	} else mInfoPanel->hide();
}

void MvdMainWindow::resetFilter()
{
	mFilterWidget->editor()->setText(QString());
	mFilterWidget->hide();
	mInfoPanel->hide();
}

bool MvdMainWindow::isQuickFilterVisible() const
{
	return mFilterWidget->isVisible();
}

void MvdMainWindow::showSharedDataEditor()
{
	MvdSharedDataEditor editor(this);
	editor.setModel(mSharedDataModel);
	editor.setWindowModality(Qt::ApplicationModal);
	editor.show();
}

//! Returns false if the quick filter should not be shown after a key press (e.g. because focus is on the SD editor)
bool MvdMainWindow::shouldShowQuickFilter() const
{
	if (!mCollection || mCollection->isEmpty())
		return false;

	QWidget* w = QApplication::focusWidget();

	// Shared data editor must not loose its focus
	while (w) {
		if (w == mSharedDataDock)
			return false;
		w = qobject_cast<QWidget*>(w->parent());
	}

	return true;
}

QMenu* MvdMainWindow::createPopupMenu()
{
	QMenu* menu = QMainWindow::createPopupMenu();
	menu->addSeparator();
	if (!menu->isEmpty() && !menu->actions().last()->isSeparator())
		menu->addSeparator();
	menu->addAction(mA_LockToolBars);
	return menu;
}

void MvdMainWindow::lockToolBars(bool lock)
{
	mTB_MainToolBar->setMovable(!lock);
}

//! Enlarges the smart view tiles.
void MvdMainWindow::zoomIn()
{
	if (mMainViewStack->currentWidget() != mSmartView)
		return;

	bool canZoomIn = true;

	MvdSmartView::ItemSize current = mSmartView->itemSize();
	switch (current) {
	case MvdSmartView::LargeItemSize: canZoomIn = false; break;
	case MvdSmartView::SmallItemSize: mSmartView->setItemSize(MvdSmartView::MediumItemSize); break;
	case MvdSmartView::MediumItemSize: mSmartView->setItemSize(MvdSmartView::LargeItemSize); canZoomIn = false; break;
	default: ;
	}

	mA_ViewModeZoomIn->setEnabled(canZoomIn);
	mA_ViewModeZoomOut->setEnabled(true);
}

//! Reduces the smart view tiles.
void MvdMainWindow::zoomOut()
{
	if (mMainViewStack->currentWidget() != mSmartView)
		return;

	bool canZoomOut = true;

	MvdSmartView::ItemSize current = mSmartView->itemSize();
	switch (current) {
	case MvdSmartView::SmallItemSize: canZoomOut = false; break;
	case MvdSmartView::LargeItemSize: mSmartView->setItemSize(MvdSmartView::MediumItemSize); break;
	case MvdSmartView::MediumItemSize: mSmartView->setItemSize(MvdSmartView::SmallItemSize); canZoomOut = false; break;
	default: ;
	}

	mA_ViewModeZoomOut->setEnabled(canZoomOut);
	mA_ViewModeZoomIn->setEnabled(true);
}

void MvdMainWindow::pluginActionTriggered()
{
	QAction* a = qobject_cast<QAction*>(sender());
	if (!a) return;

	QStringList l = a->data().toString().split(QLatin1Char('/'));
	if (l.size() != 2) return;

	QString pluginName = l[0];
	QString actionName = l[1];

	MvdPluginInterface* iface = 0;
	foreach (MvdPluginInterface* i, mPlugins) {
		QString s = i->info().uniqueId.trimmed();
		if (s.replace(QLatin1Char('/'), QLatin1Char('.')) == pluginName) {
			iface = i;
			break;
		}
	}

	if (!iface) return;

	MvdPluginContext* context = MvdCore::pluginContext();
	

	// Update plugin context
	iface->actionTriggered(actionName);

	// Consume known properties
	QHash<QString,QVariant>::Iterator it = context->properties.begin();
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

bool MvdMainWindow::collectionLoaderCallback(int state, const QVariant& data)
{
	if (state == MvdCollectionLoader::CollectionInfo) {
		
		return true;

		MvdCollectionLoader::Info info = data.value<MvdCollectionLoader::Info>();
		QString msg = tr("Collection %1 contains %2 movies. Continue?")
			.arg(info.metadata[QString("name")])
			.arg(info.expectedMovieCount);

		int res = QMessageBox::question(this, MVD_CAPTION, msg, QMessageBox::Yes, QMessageBox::No);

		return (res == QMessageBox::Yes);

	} else if (state == MvdCollectionLoader::ProgressInfo) {

		int percent = data.toInt();

	}

	return true;
}

void MvdMainWindow::setMoviePoster(quint32 movieId, const QUrl& url)
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
			statusBar()->showMessage(tr("A new movie poster has been set for '%1'.").arg(t));
		} else statusBar()->showMessage(tr("Failed to set a movie poster set for '%1'.").arg(t));

	} else {
		// Download required
		if (!mHttp) {
			mHttp = new QHttp(this);
			connect(mHttp, SIGNAL(requestFinished(int,bool)), SLOT(httpRequestFinished(int,bool)));
		}

		mHttp->setHost(url.host(), url.port(80));
		
		QString location = url.path();
		if (url.hasQuery())
			location.append("?").append(url.encodedQuery());

		QTemporaryFile* tempFile = new QTemporaryFile(paths().tempDir());
		if (!tempFile->open()) {
			statusBar()->showMessage(tr("Failed to set a movie poster set for '%1'.").arg(t));
			return;
		}

		statusBar()->showMessage(tr("Downloading movie poster for '%1'.").arg(t));
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

void MvdMainWindow::httpRequestFinished(int id, bool error)
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
			statusBar()->showMessage(tr("Failed to download movie poster for '%1'.").arg(rr.data.toString()));
			delete rr.tempFile;
			return;
		}

		QString s = mCollection->addImage(rr.tempFile->fileName());
		if (s.isEmpty()) {
			statusBar()->showMessage(tr("Failed to set a movie poster set for '%1'.").arg(rr.data.toString()));
		} else {
			m.setPoster(s);
			mCollection->updateMovie(rr.target, m);
			statusBar()->showMessage(tr("A new movie poster has been set for '%1'.").arg(rr.data.toString()));
		}

		delete rr.tempFile;
	}
	default: ;
	}
}

void MvdMainWindow::sdeDragStarted()
{
	mDraggingSharedData = true;
	mFilterWidget->show();
	mSavedFilterMessage = (int) mFilterWidget->message();
	mFilterWidget->setMessage(MvdFilterWidget::DropInfo);
	mHideFilterTimer->stop();
}

void MvdMainWindow::sdeDragEnded()
{
	mDraggingSharedData = false;
	mFilterWidget->setMessage((MvdFilterWidget::Message) mSavedFilterMessage);
	if (mFilterWidget->editor()->text().trimmed().isEmpty() && !mFilterWidget->editor()->hasFocus()) {
		resetFilter();
	}
}

MvdFilterWidget* MvdMainWindow::filterWidget() const
{
	return mFilterWidget;
}

void MvdMainWindow::infoPanelClosedByUser()
{
	mInfoPanelClosedByUser = true;
}
