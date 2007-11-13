/**************************************************************************
** Filename: mainwindow.h
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

#ifndef MVD_MAINWINDOW_H
#define MVD_MAINWINDOW_H

#include "global.h"
#include "movieeditor.h"
#include <QMainWindow>
#include <QStringList>
#include <QAbstractItemView>
#include <QPointer>

class QToolBar;
class QAction;
class QActionGroup;
class QMenuBar;
class QMenu;
class QGridLayout;
class QTextBrowser;
class QStackedWidget;
class QTimer;
class QEvent;
class QKeyEvent;
class MvdSmartView;
class MvdTreeView;
class MvdDockWidget;
class MvdMovieCollection;
class MvdCollectionModel;
class MvdFilterWidget;
class MvdFilterProxyModel;

class MvdMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MvdMainWindow(QWidget* parent = 0);
	virtual ~MvdMainWindow();

	MvdMovieCollection* currentCollection();

public slots:
	bool loadCollection(const QString& file);
	void filter(QString s);
	void resetFilter();

protected:
	void keyPressEvent(QKeyEvent* e);
	bool eventFilter(QObject* o, QEvent* e);

protected slots:
	void closeEvent(QCloseEvent* e);
	
private:
	QGridLayout* mMainLayout;

	// Toolbars
	QToolBar* mTB_File;
	QToolBar* mTB_View;
	QToolBar* mTB_Coll;
	QToolBar* mTB_Tool;
	QToolBar* mTB_Help;

	// Actions
	QAction* mA_FileExit;
	QAction* mA_FileNew;
	QAction* mA_FileOpen;
	QAction* mA_FileOpenLast;
	QAction* mA_FileSave;
	QAction* mA_FileSaveAs;
	
	QActionGroup* mAG_SortActions;
	QAction* mA_SortDescending;

	QActionGroup* mAG_MovieView;
	QAction* mA_SmartView;
	QAction* mA_TreeView;
	QAction* mA_ViewDetails;
	QAction* mA_CollAddMovie;
	QAction* mA_CollRemMovie;
	QAction* mA_CollEdtMovie;
	QAction* mA_CollDupMovie;
	QAction* mA_CollMeta;
	QAction* mA_ToolPref;
	QAction* mA_ToolLog;
	QAction* mA_HelpAbout;
	QAction* mA_HelpContents;
	QAction* mA_HelpIndex;

	// Menus
	QMenuBar* mMB_MenuBar;

	QMenu* mMN_File;
	QMenu* mMN_FileMRU;
	QMenu* mMN_FileImport;
	QMenu* mMN_View;
	QMenu* mMN_ViewSort;
	QMenu* mMN_Coll;
	QMenu* mMN_Tool;
	QMenu* mMN_Help;
	QMenu* mMN_Plugins;
	
	// Views
	QStackedWidget* mMainViewStack;
	MvdSmartView* mSmartView;
	MvdTreeView* mTreeView;
	QTextBrowser* mDetailsView;

	// Dock windows
	MvdDockWidget* mDetailsDock;
	
	MvdFilterWidget* mFilterWidget;
	QTimer* mHideFilterTimer;
	MvdFilterProxyModel* mFilterModel;
	
	// The current movie collection
	MvdMovieCollection* mCollection;
	MvdCollectionModel* mMovieModel;

	QPointer<MvdMovieEditor> mMovieEditor;

	void setupUi();
	void retranslateUi();
	void setupConnections();
	void createNewCollection();
	quint32 modelIndexToId(const QModelIndex& index) const;

private slots:
	bool closeCollection();
	bool loadCollectionDlg();
	bool saveCollection();
	bool saveCollectionDlg();
	void addMovie();
	void addRecentFile(const QString& file);
	void applyCurrentFilter();
	void collectionModelSorted();
	void collectionModified();
	void currentViewChanged();
	void cycleMovieView();
	void dockViewsToggled();
	void duplicateCurrentMovie();
	void editCurrentMovie();
	void editMovie(const QModelIndex& index);
	void editNextMovie();
	void editPreviousMovie();
	void externalActionTriggered(const QString& id, const QVariant& data);
	void loadLastCollection();
	void loadPlugins();
	void loadPluginsFromDir(const QString& path);
	void movieChanged(mvdid);
	void movieViewSelectionChanged();
	void movieViewToggled(QAction*);
	void newCollection();
	void openRecentFile(QAction* a);
	void removeCurrentMovie();
	void removeMovie(const QModelIndex& index);
	void removeMovies(const QModelIndexList& list);
	void showFilterWidget();
	void showLog();
	void showMetaEditor();
	void showMovieContextMenu(const QModelIndex& index);
	void showPreferences();
	void sortActionTriggered(QAction* a = 0);
	void treeViewSorted(int logicalIndex);
	void updateCaption();
	void updateDetailsView();
	void updateFileMenu();
	void updateViewSortMenu();
};

namespace Movida {
	extern MvdMainWindow* MainWindow;
}

#endif // MVD_MAINWINDOW_H
