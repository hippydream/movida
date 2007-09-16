/**************************************************************************
** Filename: sdtreewidget.h
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

#ifndef MVD_SDTREEWIDGET_H
#define MVD_SDTREEWIDGET_H

#include "treewidget.h"
#include "movie.h"
#include "moviecollection.h"
#include "shareddata.h"

#include <QItemDelegate>

class MvdSDTreeWidget : public MvdTreeWidget
{
	Q_OBJECT
  
public:
	//! \internal
	enum ActionType
	{
		RemoveItemAction, ChangeItemAction,
		AddItemAction, AddNewItemAction,
		ShowEditorAction, ClearAction, ShowPropertyEditorAction, 
		ExecuteItemAction, ShowItemSelectorAction,
		NoAction
	};

	//! \internal
	typedef struct ActionDescriptor
	{
		ActionDescriptor() : type(NoAction), itemId(0), current(false) {}
		ActionDescriptor(ActionType t, quint32 i, bool c = false) : 
			type(t), itemId(i), current(c) {}
		ActionType type;
		quint32 itemId;
		bool current;
	};

	MvdSDTreeWidget(QWidget* parent = 0);
	MvdSDTreeWidget(Movida::SmdDataRole ds, const MvdMovie& movie, 
		MvdMovieCollection* c, QWidget* parent = 0);

	void setDataSource(Movida::SmdDataRole ds);
	Movida::SmdDataRole dataSource() const { return mDS; }

	void resetToDefaults();

	void setMovie(const MvdMovie& m);
	void store(MvdMovie& m);
	
	void setMovieCollection(MvdMovieCollection* mc);
	MvdMovieCollection* movieCollection() const { return mCollection; }

	QList<quint32> currentValues(quint32 excluded = 0, bool excludeNewItems = true) const;

	bool isModified() const;

private slots:
	void showContextMenu(QTreeWidgetItem* item, int col);
	void itemDoubleClicked(QTreeWidgetItem* item);

private:
	MvdMovie mMovie;
	MvdMovieCollection* mCollection;
	Movida::SmdDataRole mDS;
	bool mMaybeModified;

	void init();
	void setPersonRoleData(const QHash<smdid, QStringList>& d);
	void setSimpleData(const QList<smdid>& d, Movida::SmdDataRole ds);

	inline QMenu* createItemMenu(const QString& label, 
		const QMap<QString,ActionDescriptor>& actions, ActionType type);
	
	inline QString joinStringList(const QStringList& list, const QString& sep, 
		const QString& def = QString()) const;
	inline QStringList splitString(const QString& s) const;
	
	inline void executeAction(ActionDescriptor ad, QTreeWidgetItem* item = 0,
		const QVariant& data = QVariant());

	inline QMap<QString,ActionDescriptor> generateActions(quint32 selected = 0, 
		int* itemCount = 0, int max = -1);
	inline void generateActions(const QHash<smdid, MvdSharedData::PersonData>& d, 
		const QList<quint32>& current, QMap<QString,ActionDescriptor>& actions,
		quint32 selected = 0);
	inline void generateActions(const QHash<smdid, MvdSharedData::StringData>& d, 
		const QList<quint32>& current, QMap<QString,ActionDescriptor>& actions, 
		quint32 selected = 0);
	inline void generateActions(const QHash<smdid, MvdSharedData::UrlData>& d, 
		const QList<quint32>& current, QMap<QString,ActionDescriptor>& actions, 
		quint32 selected = 0);

	void keyPressEvent(QKeyEvent* event);

	MvdSharedData& smd(smdid id) const
	{
		Q_ASSERT(mCollection != 0);

		if (MvdSharedData::isHardcoded(id))
			return Movida::globalSD();
		return mCollection->smd();
	}

	friend class MvdSDDelegate;
};
Q_DECLARE_METATYPE(MvdSDTreeWidget::ActionDescriptor)

class MvdSDDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	MvdSDDelegate(MvdSDTreeWidget* parent = 0);

	QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, 
		const QModelIndex& index) const;
	void setEditorData(QWidget* editor, const QModelIndex& index) const;
	void setModelData(QWidget* editor, QAbstractItemModel* model, 
		const QModelIndex& index) const;
	void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, 
		const QModelIndex& index) const;

	bool eventFilter(QObject* object, QEvent* event);
	bool editorEvent(QEvent* event, QAbstractItemModel* model, 
		const QStyleOptionViewItem& option, const QModelIndex& index);

private:
	enum DelegateType
	{
		//! Standard SMD data populated combo
		StandardDelegate, 
		//! One combo for both first and last name
		PersonDelegate, 
		//! Trivial line edit
		TextDelegate,
		//! Parent is not a valid MvdSDTreeWidget
		UnsupportedDelegate
	};

	inline DelegateType delegateType(const QModelIndex& index, 
		MvdSDTreeWidget** tree = 0) const;
	inline bool isItemValid(Movida::SmdDataRole ds, 
		const QTreeWidgetItem& item) const;
};

#endif // MVD_SDTREEWIDGET_H
