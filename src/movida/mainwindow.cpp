/**************************************************************************
** Filename: mainwindow.cpp
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

#include "mainwindow.h"
#include "mainwindow_p.h"

#include "collectionmetaeditor.h"
#include "collectionmodel.h"
#include "dockwidget.h"
#include "filterproxymodel.h"
#include "filterwidget.h"
#include "guiglobal.h"
#include "infopanel.h"
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
#include "mvdcore/pathresolver.h"
#include "mvdcore/plugininterface.h"
#include "mvdcore/settings.h"
#include "mvdcore/templatemanager.h"

#include "mvdshared/browserview.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QLibrary>
#include <QtCore/QList>
#include <QtCore/QSignalMapper>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtCore/QtDebug>
#include <QtGui/QActionGroup>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QListView>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QTextBrowser>
#include <QtGui/QToolBar>
#include <QtNetwork/QHttp>

using namespace Movida;
MvdMainWindow *Movida::MainWindow = 0;

Q_DECLARE_METATYPE(MvdCollectionLoader::Info);


//////////////////////////////////////////////////////////////////////////

/*!
    \class MvdMainWindow mainwindow.h
    \ingroup Movida

    \brief The Movida main window.
*/

//! \todo Option to always exclude loaned movies from searches

/*!
    Creates a new main window and initializes the application.
*/
MvdMainWindow::MvdMainWindow(QWidget *parent) :
    QMainWindow(parent),
    d(new Private(this))
{
    Q_ASSERT_X(!Movida::MainWindow, "MvdMainWindow",
        "Internal Error. Only a single instance of MvdMainWindow can be created.");
    Movida::MainWindow = this;

    MVD_WINDOW_ICON

    d->mMovieModel = new MvdCollectionModel(this);
    d->mSharedDataModel = new MvdSharedDataModel(Movida::PersonRole, this);

    d->setupUi();
    d->setupConnections();


    // Set some GUI related constants
    QHash<QString, QVariant> parameters;
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
    MvdSettings &p = Movida::settings();

    // Set default settings
    p.setDefaultValue("movida/maximum-recent-files", 5);
    p.setDefaultValue("movida/confirm-delete-movie", true);
    p.setDefaultValue("movida/directories/remember", true);
    p.setDefaultValue("movida/movie-list/initials", false);    //! \todo: rename to movie-view
    p.setDefaultValue("movida/use-history", true);
    p.setDefaultValue("movida/max-history-items", 20);
    p.setDefaultValue("movida/quick-filter/case-sensitive", false);
    p.setDefaultValue("movida/quick-filter/attributes", d->mFilterModel->quickFilterAttributes());
    p.setDefaultValue("movida/view-mode", "smart");
    p.setDefaultValue("movida/smart-view/item-size", "medium");
    p.setDefaultValue("movida/movie-view/wheel-up-magnifies", true);

    // Initialize core library && load user settings
    QStringList recentFiles = p.value("movida/recent-files").toStringList();
    int max = p.value("movida/maximum-recent-files").toInt();
    int recentFilesCount = recentFiles.size();
    while (recentFiles.size() > max)
        recentFiles.removeLast();
    if (recentFilesCount != recentFiles.size())
        p.setValue("movida/recent-files", recentFiles);

    d->mA_FileOpenLast->setDisabled(recentFiles.isEmpty());

    QString viewMode = p.value("movida/view-mode").toString();
    if (viewMode == "smart")
        d->mA_ViewModeSmart->activate(QAction::Trigger);
    else
        d->mA_ViewModeTree->activate(QAction::Trigger);

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

    d->mFilterWidget->setCaseSensitivity(p.value("movida/quick-filter/case-sensitive").toBool()
        ? Qt::CaseSensitive : Qt::CaseInsensitive);
    d->mFilterModel->setQuickFilterAttributes(p.value("movida/quick-filter/attributes").toByteArray());

    // a new empty collection is always open at startup
    d->mA_FileNew->setDisabled(true);
    d->mA_FileSave->setDisabled(true);
    d->mA_FileSaveAs->setDisabled(true);

    d->movieViewSelectionChanged();
    d->mTreeView->sortByColumn((int)Movida::TitleAttribute, Qt::AscendingOrder);
    d->mSharedDataModel->sort(0, Qt::AscendingOrder);
    d->currentViewChanged();

    setAcceptDrops(true);

    d->createNewCollection();
    d->loadPlugins();
    d->registerMessageHandler();

    d->updateCaption();

    statusBar()->showMessage(tr("Movida is ready!"));
    iLog() << "Movida is ready!";
}

/*!
    \todo Review evtl. needed deletes
*/
MvdMainWindow::~MvdMainWindow()
{ }

/*!
    Stores preferences and application status.
*/
void MvdMainWindow::closeEvent(QCloseEvent *e)
{
    iLog() << "Movida is closing...";

    if (d->mCollection && d->mCollection->isModified()) {
        if (!d->closeCollection()) {
            e->ignore();
            return;
        }
    }

    cleanUp();
}

//! This method is called before closing the app or on a crash.
void MvdMainWindow::cleanUp()
{
    d->cleanUp();
}

void MvdMainWindow::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();
    QString ttf = d->mFilterWidget->editor()->text();
    QString text = e->text();

    bool hasModifier = !(e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ALT
                         || e->modifiers().testFlag(Qt::ShiftModifier));

    if (hasModifier || !d->shouldShowQuickFilter()) {
        QMainWindow::keyPressEvent(e);
        return;
    }

    if (d->mFilterWidget->isVisible()) {
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
            d->mFilterWidget->editor()->clear();
            return;
        }
        ttf = text;
    }

    d->mFilterWidget->editor()->setText(ttf);
}

//! Converts a collection model index to a movie ID.
mvdid MvdMainWindow::movieIndexToId(const QModelIndex &index) const
{
    return d->movieIndexToId(index);
}

//! Converts a movie ID to a collection model index.
QModelIndex MvdMainWindow::movieIdToIndex(mvdid id) const
{
    return d->movieIdToIndex(id);
}

//! Enlarges the smart view tiles.
void MvdMainWindow::zoomIn()
{
    if (d->mMainViewStack->currentWidget() != d->mSmartView)
        return;

    bool canZoomIn = true;

    MvdSmartView::ItemSize current = d->mSmartView->itemSize();
    switch (current) {
        case MvdSmartView::LargeItemSize:
            canZoomIn = false; break;

        case MvdSmartView::SmallItemSize:
            d->mSmartView->setItemSize(MvdSmartView::MediumItemSize); break;

        case MvdSmartView::MediumItemSize:
            d->mSmartView->setItemSize(MvdSmartView::LargeItemSize); canZoomIn = false; break;

        default:
            ;
    }

    d->mA_ViewModeZoomIn->setEnabled(canZoomIn);
    d->mA_ViewModeZoomOut->setEnabled(true);
}

//! Reduces the smart view tiles.
void MvdMainWindow::zoomOut()
{
    if (d->mMainViewStack->currentWidget() != d->mSmartView)
        return;

    bool canZoomOut = true;

    MvdSmartView::ItemSize current = d->mSmartView->itemSize();
    switch (current) {
        case MvdSmartView::SmallItemSize:
            canZoomOut = false; break;

        case MvdSmartView::LargeItemSize:
            d->mSmartView->setItemSize(MvdSmartView::MediumItemSize); break;

        case MvdSmartView::MediumItemSize:
            d->mSmartView->setItemSize(MvdSmartView::SmallItemSize); canZoomOut = false; break;

        default:
            ;
    }

    d->mA_ViewModeZoomOut->setEnabled(canZoomOut);
    d->mA_ViewModeZoomIn->setEnabled(true);
}

//! Clears any active filter and hides the filter bar and info panel.
void MvdMainWindow::resetFilter()
{
    d->mFilterWidget->editor()->setText(QString());
    d->mFilterWidget->hide();
    d->mInfoPanel->hide();
}

bool MvdMainWindow::eventFilter(QObject *o, QEvent *e)
{
    if (o == d->mFilterWidget->editor()) {
        if (e->type() == QEvent::FocusIn && d->mHideFilterTimer->isActive())
            d->mHideFilterTimer->stop();
    } else if (o == d->mSmartView->viewport() || o == d->mTreeView->viewport()) {
        if (e->type() == QEvent::DragEnter && !d->mDraggingSharedData) {
            QDragEnterEvent *_e = static_cast<QDragEnterEvent *>(e);
            if (_e->source() == d->mSharedDataEditor->view()) {
                // Dirty dirty dirty trick to detect the end of a drag operation.
                // Any attempt to use DragLeave or Drop failed.
                connect(_e->mimeData(), SIGNAL(destroyed()), d, SLOT(sdeDragEnded()));
                d->sdeDragStarted();
            }
        } else if (e->type() == QEvent::Wheel && o == d->mSmartView->viewport()) {
            if (qApp->keyboardModifiers() == Qt::ControlModifier) {
                QWheelEvent *we = static_cast<QWheelEvent *>(e);

                bool zoomInReq = we->delta() >= 0;
                if (!Movida::settings().value("movida/movie-view/wheel-up-magnifies").toBool())
                    zoomInReq = !zoomInReq; // Invert direction

                QAction *za = zoomInReq ? d->mA_ViewModeZoomIn : d->mA_ViewModeZoomOut;

                int numDegrees = qAbs(we->delta()) / 8;
                int numSteps = numDegrees / 15;

                for (int i = 0; i < numSteps && za->isEnabled(); ++i) {
                    if (zoomInReq)
                        zoomIn();
                    else zoomOut();
                }
                return true; // Filter out or the view will scroll!
            }
        }
    }

    return QMainWindow::eventFilter(o, e);
}

void MvdMainWindow::lockToolBars(bool lock)
{
    d->mTB_MainToolBar->setMovable(!lock);
}

//! Returns the current collection. This method will create a new collection if none exists.
MvdMovieCollection *MvdMainWindow::currentCollection() const
{
    if (!d->mCollection)
        const_cast<MvdMainWindow *>(this)->d->createNewCollection();
    Q_ASSERT(d->mCollection);
    return d->mCollection;
}

QList<mvdid> MvdMainWindow::selectedMovies() const
{
    QModelIndexList list = d->mTreeView->selectedRows();

    QList<mvdid> ids;
    for (int i = 0; i < list.size(); ++i)
        ids << movieIndexToId(list.at(i));
    return ids;
}

/*!
    Closes the current collection and creates a new collection.
    The user will be asked to save the current collection if necessary.
*/
void MvdMainWindow::newCollection()
{
    if (!d->mCollection)
        d->createNewCollection();
    Q_ASSERT(d->mCollection);

    if (d->closeCollection())
        d->mA_FileNew->setDisabled(true);
}

/*!
    Loads a new collection from a file.
    The user will be asked to save the current collection if necessary.
*/
bool MvdMainWindow::loadCollection(const QString &file)
{
    if (!d->closeCollection())
        return true;

    /*! \todo add thread to load coll. and (_IMPORTANT_) lock gui!*/

    if (!d->mCollection)
        d->createNewCollection();

    // Avoid the colelctionModified() signal emission.
    d->mCollection->setModifiedStatus(true);

    setEnabled(false);

    MvdCollectionLoader loader;
    loader.setProgressHandler(d, "collectionLoaderCallback");
    MvdCollectionLoader::StatusCode res = loader.load(d->mCollection, file);
    d->mCollection->setModifiedStatus(false);

    if (res != MvdCollectionLoader::NoError) {
        d->createNewCollection();
        QMessageBox::warning(this, MVD_CAPTION, tr("Failed to load the collection."));
    }

    setEnabled(true);

    if (res != MvdCollectionLoader::NoError)
        return false;

    // Update GUI controls
    d->collectionModified();

    Movida::MovieAttribute a = Movida::TitleAttribute;
    QAction *ca = d->mAG_ViewSort->checkedAction();
    if (ca)
        a = (Movida::MovieAttribute)ca->data().toInt();

    d->mTreeView->sortByColumn((int)a, d->mFilterModel->sortOrder());
    d->mSharedDataModel->sort(0, d->mSharedDataModel->sortOrder());

    d->mInfoPanel->showTemporaryMessage(tr("%1 movies loaded.").arg(d->mCollection->count()));

    return true;
}

/*!
    Re-opens the last closed collection.
    The user will be asked to save the current collection if necessary.
*/
void MvdMainWindow::loadLastCollection()
{
    QStringList recentFiles =
        Movida::settings().value("movida/recent-files").toStringList();

    if (recentFiles.isEmpty())
        return;

    loadCollection(recentFiles.at(0));
}

//! \todo Move plugin handling to mvdcore library and add a hook to register UI actions.
MvdPluginInterface *MvdMainWindow::locatePlugin(const QString &id) const
{
    foreach(MvdPluginInterface * iface, d->mPlugins)
    {
        if (iface->id() == id) {
            return iface;
        }
    }
    return 0;
}

/*!
    Saves the current collection.
*/
bool MvdMainWindow::saveCollection()
{
    Q_ASSERT(d->mCollection);
    Q_ASSERT(!d->mCollection->isEmpty());

    MvdCollectionSaver saver(this);
    MvdCollectionSaver::StatusCode res = saver.save(d->mCollection);
    if (res != MvdCollectionSaver::NoError) {
        QMessageBox::warning(this, MVD_CAPTION, tr("Failed to save the collection."));
        return false;
    }

    d->mCollection->setModifiedStatus(false);
    d->mA_FileSave->setEnabled(false);
    d->updateCaption();

    return true;
}

void MvdMainWindow::clearRecentFiles()
{
    Movida::settings().setValue("movida/recent-files", QStringList());
    d->mA_FileOpenLast->setEnabled(false);
}

void MvdMainWindow::filter(QString s)
{
    d->filter(s);
}

QMenu *MvdMainWindow::createPopupMenu()
{
    QMenu *menu = QMainWindow::createPopupMenu();

    menu->addSeparator();
    if (!menu->isEmpty() && !menu->actions().last()->isSeparator())
        menu->addSeparator();
    menu->addAction(d->mA_LockToolBars);
    return menu;
}

MvdFilterWidget *MvdMainWindow::filterWidget() const
{
    return d->mFilterWidget;
}

void MvdMainWindow::handleLinkClicked(const QUrl &url)
{
    if (url.scheme() == QLatin1String("http")) {
        QDesktopServices::openUrl(url);
        return;
    }

    MvdActionUrl aurl = MvdCore::parseActionUrl(url);
    if (!aurl.isValid()) {
        Movida::wLog() << "MainWindow: malformed action URL: " << aurl;
        return;
    }

    if (aurl.action == QLatin1String("plugin")) {
        QStringList sl = aurl.parameter.split(QChar('/'), QString::SkipEmptyParts);
        if (sl.size() < 2) {
            Movida::wLog() << "MainWindow: invalid action URL: " << aurl;
            return;
        }

        QString pluginId = sl.at(0);
        MvdPluginInterface *plugin = locatePlugin(pluginId);
        if (!plugin) {
            Movida::wLog() << "MainWindow: failed to dispatch action URL " << aurl << " to plugin " << pluginId;
            return;
        }

        Movida::iLog() << "MainWindow: dispatching action URL " << aurl << " to plugin " << pluginId;
        QString action = sl.at(1);
        sl.removeAt(0);     // Remove plugin id
        sl.removeAt(0);     // Remove action name
        plugin->actionTriggered(action, sl);
        return;
    }

    if (aurl.action != QLatin1String("movida")) {
        Movida::wLog() << "MainWindow: unsupported action URL: " << aurl;
        return;
    }

    QStringList sl = aurl.parameter.split(QChar('/'), QString::SkipEmptyParts);
    if (sl.isEmpty()) {     // We require at least an action
        Movida::wLog() << "MainWindow: invalid action URL: " << aurl;
        return;
    }

    bool handled = false;
    QString action = sl.takeAt(0);
    if (action == QLatin1String("collection") && !sl.isEmpty()) {
        action = sl.takeAt(0);
        if (action == QLatin1String("add")) {
            addMovie();
            handled = true;
        } else if (action == QLatin1String("about")) {
            showCollectionMeta();
            handled = true;
        }
    }
}

/*!
    Shows the preferences dialog.
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
    QFileInfo fi(paths().logFile());
    QTextBrowser *viewer = new QTextBrowser;

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
    Shows a dialog to add a new movie.
*/
void MvdMainWindow::addMovie()
{
    //! \todo Ensure mCollection is never 0
    MvdMovieEditor pd(d->mCollection, this);

    pd.exec();

    QModelIndex index = movieIdToIndex(pd.movieId());

    // Select the new movie
    if (index.isValid()) {
        QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect;
        d->mSelectionModel->select(index, flags);
    }
    d->collectionModified();
}

void MvdMainWindow::duplicateCurrentMovie()
{
    mvdid id = movieIndexToId(d->mTreeView->selectedIndex());

    if (id == 0)
        return;

    MvdMovie movie = d->mCollection->movie(id);
    if (!movie.isValid())
        return;

    bool ok;
    QString text = QInputDialog::getText(this, MVD_CAPTION,
        tr("Please enter a (unique) title for the new movie:"), QLineEdit::Normal,
        movie.title(), &ok);

    if (!ok || text.isEmpty())
        return;

    //! \todo Validate title for duplicated movie
    if (text.toLower() == movie.title().toLower()) {
        QMessageBox::warning(this, MVD_CAPTION, tr("Sorry, but the new title must be unique in this collection."));
        return;
    }

    movie.setTitle(text);
    if (d->mCollection->addMovie(movie) == 0)
        QMessageBox::warning(this, MVD_CAPTION, tr("Sorry but the movie could not be duplicated."));
}

void MvdMainWindow::editMovie(const QModelIndex &index)
{
    mvdid id = movieIndexToId(index);
    bool resetRequired = false;
    bool exec = d->mMovieEditor.isNull();

    if (exec) {
        d->mMovieEditor = new MvdMovieEditor(d->mCollection, this);
        d->mMovieEditor->setAttribute(Qt::WA_DeleteOnClose, true);
    } else
        resetRequired = true;

    if (!d->mMovieEditor->setMovie(id, true))
        return;

    d->mTreeView->setCurrentIndex(index);

    if (resetRequired) {
        // Reset state
        d->mMovieEditor->disconnect(this);
        d->mMovieEditor->setPreviousEnabled(false);
        d->mMovieEditor->setNextEnabled(false);
    }

    QModelIndex i;
    bool prev = false;
    bool next = false;

    i = d->mTreeView->indexAbove(index);
    if (i.isValid()) {
        id = movieIndexToId(i);
        if (id != 0) {
            d->mMovieEditor->setPreviousEnabled(true);
            prev = true;
        }
    }

    i = d->mTreeView->indexBelow(index);
    if (i.isValid()) {
        id = movieIndexToId(i);
        if (id != 0) {
            d->mMovieEditor->setNextEnabled(true);
            next = true;
        }
    }

    if (next)
        connect(d->mMovieEditor, SIGNAL(nextRequested()), this, SLOT(editNextMovie()));
    if (prev)
        connect(d->mMovieEditor, SIGNAL(previousRequested()), this, SLOT(editPreviousMovie()));

    if (exec)
        d->mMovieEditor->exec();
}

void MvdMainWindow::editNextMovie()
{
    QModelIndex next = d->mTreeView->indexBelow(d->mTreeView->selectedIndex());

    editMovie(next);
}

void MvdMainWindow::editPreviousMovie()
{
    QModelIndex prev = d->mTreeView->indexAbove(d->mTreeView->selectedIndex());

    editMovie(prev);
}

void MvdMainWindow::editSelectedMovies()
{
    QModelIndexList list = d->mTreeView->selectedRows();

    if (list.isEmpty()) {
        return;
    } else if (list.size() == 1) {
        editMovie(list.at(0));
    } else {
        // TEMPORARY
        QInputDialog input(this);
        input.setInputMode(QInputDialog::TextInput);
        input.setLabelText(tr("Storage ID"));
        int exitCode = input.exec();
        if (exitCode == QDialog::Accepted) {
            QString value = input.textValue();
            for (int i = 0; i < list.size(); ++i) {
                mvdid id = movieIndexToId(list.at(i));
                if (id != MvdNull) {
                    MvdMovie m = d->mCollection->movie(id);
                    m.setStorageId(value);
                    d->mCollection->updateMovie(id, m);
                }
            }
        }
    }
}

void MvdMainWindow::massEditSelectedMovies()
{
    QList<mvdid> ids = selectedMovies();
    if (ids.size() > 1) {
        MvdMovieMassEditor med(d->mCollection, this);
        med.setMovies(ids);
        med.exec();
    }
}

void MvdMainWindow::removeMovie(const QModelIndex &index)
{
    mvdid id = movieIndexToId(index);

    if (id == 0)
        return;

    bool confirmOk = Movida::settings().value("movida/confirm-delete-movie").toBool();
    if (confirmOk) {
        QString msg = tr("Are you sure you want to delete this movie?");

        int res = QMessageBox::question(this, MVD_CAPTION, msg,
            QMessageBox::Yes, QMessageBox::No);
        if (res != QMessageBox::Yes)
            return;
    }

    d->mCollection->removeMovie(id);

    d->movieViewSelectionChanged();
    d->collectionModified();
}

void MvdMainWindow::removeMovies(const QModelIndexList &list)
{
    bool confirmOk = Movida::settings().value("movida/confirm-delete-movie").toBool();

    if (confirmOk) {
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
        d->mCollection->removeMovie(ids.at(i));
    }

    d->movieViewSelectionChanged();
    d->collectionModified();
}

void MvdMainWindow::removeSelectedMovies()
{
    QModelIndexList list = d->mTreeView->selectedRows();

    if (list.isEmpty())
        return;

    removeMovies(list);
}

void MvdMainWindow::showMovieContextMenu(const QModelIndex &index)
{
    d->showMovieContextMenu(index);
}

//! Shows the collection metadata editor.
void MvdMainWindow::showCollectionMeta()
{
    //! Handle read-only collections
    MvdCollectionMetaEditor editor(this);

    editor.setCollection(currentCollection());
    editor.exec();
}

void MvdMainWindow::showFilterWidget()
{
    d->mFilterWidget->show();
    d->mFilterWidget->setMessage(MvdFilterWidget::NoMessage);
    d->mFilterWidget->editor()->setFocus(Qt::ShortcutFocusReason);
    d->mFilterWidget->editor()->selectAll();
    d->mHideFilterTimer->stop();
}

void MvdMainWindow::showSharedDataEditor()
{
    MvdSharedDataEditor editor(this);

    editor.setModel(d->mSharedDataModel);
    editor.setWindowModality(Qt::ApplicationModal);
    editor.show();
}

void MvdMainWindow::setMoviePoster(mvdid movieId, const QUrl &url)
{
    d->setMoviePoster(movieId, url);

}

bool MvdMainWindow::isQuickFilterVisible() const
{
    return d->isQuickFilterVisible();
}
