/**************************************************************************
** Filename: mainwindow.h
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

#ifndef MVD_MAINWINDOW_H
#define MVD_MAINWINDOW_H

#include "movieeditor.h"
#include "mvdcore/global.h"
#include <QMainWindow>
#include <QStringList>
#include <QAbstractItemView>
#include <QPointer>
#include <QUrl>
#include <QtGlobal>

class MvdCollectionModel;
class MvdDockWidget;
class MvdInfoPanel;
class MvdFilterProxyModel;
class MvdFilterWidget;
class MvdMovieCollection;
class MvdPluginInterface;
class MvdRowSelectionModel;
class MvdSharedDataEditor;
class MvdSharedDataModel;
class MvdSmartView;
class MvdMovieTreeView;
class QAction;
class QActionGroup;
class QEvent;
class QGridLayout;
class QHttp;
class QKeyEvent;
class QMenu;
class QMenuBar;
class QStackedWidget;
class QTemporaryFile;
class QTextBrowser;
class QTimer;
class QToolBar;
class QUrl;

class MvdMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MvdMainWindow(QWidget* parent = 0);
	virtual ~MvdMainWindow();

	MvdMovieCollection* currentCollection();
	void cleanUp();

	bool isQuickFilterVisible() const;
	MvdFilterWidget* filterWidget() const;

	virtual QMenu* createPopupMenu();

public slots:
	bool loadCollection(const QString& file);
	void filter(QString s);
	void filter();
	void resetFilter();

protected:
	void keyPressEvent(QKeyEvent* e);
	bool eventFilter(QObject* o, QEvent* e);

protected slots:
	void closeEvent(QCloseEvent* e);
	
private:
	QGridLayout* mMainLayout;

	// Actions
	QAction* mA_FileExit;
	QAction* mA_FileNew;
	QAction* mA_FileOpen;
	QAction* mA_FileOpenLast;
	QAction* mA_FileImport;
	QAction* mA_FileRecent;
	QAction* mA_FileSave;
	QAction* mA_FileSaveAs;

	QAction* mA_ViewModeTree;
	QAction* mA_ViewModeSmart;
	QAction* mA_ViewModeZoom;
	QAction* mA_ViewModeZoomIn;
	QAction* mA_ViewModeZoomOut;
	QActionGroup* mAG_ViewMode;
	QAction* mA_ViewDetails;
	QAction* mA_ViewSort;
	QActionGroup* mAG_ViewSort;
	QAction* mA_ViewSortDescending;

	QAction* mA_CollAddMovie;
	QAction* mA_CollRemMovie;
	QAction* mA_CollEdtMovie;
	QAction* mA_CollMedMovie;
	QAction* mA_CollDupMovie;
	QAction* mA_CollMeta;

	QAction* mA_ToolSdEditor;
	QAction* mA_ToolPref;
	QAction* mA_ToolLog;

	QAction* mA_HelpAbout;
	QAction* mA_HelpContents;
	QAction* mA_HelpIndex;

	QAction* mA_LockToolBars;

	// Top level menus
	QMenu* mMN_File;
	QMenu* mMN_View;
	QMenu* mMN_Collection;
	QMenu* mMN_Plugins;
	QMenu* mMN_Tools;
	QMenu* mMN_Help;

	// Sub menus
	QMenu* mMN_FileMRU;
	QMenu* mMN_FileImport;
	QMenu* mMN_ViewSort;

	// Tool bars
	QToolBar* mTB_MainToolBar;

	// Info panel
	MvdInfoPanel* mInfoPanel;
	bool mInfoPanelClosedByUser;
	
	// Views
	QStackedWidget* mMainViewStack;
	MvdSmartView* mSmartView;
	MvdMovieTreeView* mTreeView;
	QTextBrowser* mDetailsView;
	MvdSharedDataEditor* mSharedDataEditor;

	// Dock windows
	MvdDockWidget* mDetailsDock;
	MvdDockWidget* mSharedDataDock;
	
	MvdFilterWidget* mFilterWidget;
	QTimer* mHideFilterTimer;
	MvdFilterProxyModel* mFilterModel;
	
	// The current movie collection
	MvdMovieCollection* mCollection;
	MvdCollectionModel* mMovieModel;
	MvdRowSelectionModel* mSelectionModel;

	// SD model
	MvdSharedDataModel* mSharedDataModel;

	QPointer<MvdMovieEditor> mMovieEditor;

	QList<MvdPluginInterface*> mPlugins;

	QHttp* mHttp;

	struct RemoteRequest {
		enum { Invalid = 0, MoviePoster };

		RemoteRequest() : requestType(Invalid), requestId(-1), target(MvdNull), tempFile(0) {}

		QUrl url;
		quint16 requestType;
		int requestId;
		QVariant data;
		mvdid target;
		QTemporaryFile* tempFile;
	};
	QList<RemoteRequest> mPendingRemoteRequests;

	// D&D
	bool mDraggingSharedData;
	int mSavedFilterMessage;

	void createActions();
	QAction* createAction();
	void initAction(QAction* action, const QString& text, const QString& shortInfo, const QString& longInfo, const QString& shortcut = QString());
	void createMenus();
	void createToolBars();
	void setupUi();
	void retranslateUi();
	void setupConnections();
#ifdef MVD_DEBUG_TOOLS
	void setupDebugTools();
#endif
	void createNewCollection();
	mvdid movieIndexToId(const QModelIndex& index) const;
	QModelIndex movieIdToIndex(mvdid id) const;
	bool shouldShowQuickFilter() const;

private slots:
	bool closeCollection();
	bool collectionLoaderCallback(int state, const QVariant& data);
	bool loadCollectionDlg();
	bool saveCollection();
	bool saveCollectionDlg();
	void addMovie();
	void addRecentFile(const QString& file);
	void collectionModelSorted();
	void collectionModified();
	void currentViewChanged();
	void duplicateCurrentMovie();
	void editMovie(const QModelIndex& index);
	void editNextMovie();
	void editPreviousMovie();
	void editSelectedMovies();
	void externalActionTriggered(const QString& id, const QVariant& data);
	void httpRequestFinished(int id, bool error);
	void infoPanelClosedByUser();
	void loadLastCollection();
	void loadPlugins();
	void loadPluginsFromDir(const QString& path);
	void lockToolBars(bool lock);
	void massEditSelectedMovies();
	void movieChanged(mvdid);
	void movieViewSelectionChanged();
	void movieViewToggled(QAction*);
	void newCollection();
	void openRecentFile(QAction* a);
	void pluginActionTriggered();
	void removeMovie(const QModelIndex& index);
	void removeMovies(const QModelIndexList& list);
	void removeSelectedMovies();
	void sdeDragStarted();
	void sdeDragEnded();
	void setMoviePoster(quint32 movieId, const QUrl& url);
	void showFilterWidget();
	void showLog();
	void showMetaEditor();
	void showMovieContextMenu(const QModelIndex& index);
	void showPreferences();
	void showSharedDataEditor();
	void sortActionTriggered(QAction* a = 0);
	void treeViewSorted(int logicalIndex);
	void unloadPlugins();
	void updateCaption();
	void updateDetailsView();
	void updateFileMenu();
	void updateViewSortMenu();
	void zoomIn();
	void zoomOut();
};

namespace Movida {
	extern MvdMainWindow* MainWindow;
}

#endif // MVD_MAINWINDOW_H
