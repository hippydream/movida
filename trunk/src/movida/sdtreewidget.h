/**************************************************************************
** Filename: sdtreewidget.h
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

#ifndef MVD_SDTREEWIDGET_H
#define MVD_SDTREEWIDGET_H

#include "treewidget.h"

#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/shareddata.h"

#include <QtGui/QItemDelegate>

class MvdSDTreeWidget : public MvdTreeWidget
{
    Q_OBJECT

public:
    //! \internal
    enum ActionType {
        AddItemAction, RemoveItemAction,
        ShowEditorAction, ShowItemSelectorAction,
        NoAction
    };

    //! \internal
    struct ActionDescriptor {
        ActionDescriptor() :
            type(NoAction),
            current(false) { }

        ActionDescriptor(ActionType atype) :
            type(atype),
            current(false) { }

        ActionDescriptor(ActionType atype, mvdid aid, bool acurrent = false) :
            type(atype),
            itemIds(QList<mvdid>() << aid),
            current(acurrent) { }

        ActionDescriptor(ActionType atype, QList<mvdid> aids, bool acurrent = false) :
            type(atype),
            itemIds(aids),
            current(acurrent) { }

        ActionDescriptor(ActionType atype, QList<QTreeWidgetItem *> aitems, bool acurrent = false) :
            type(atype),
            items(aitems),
            current(acurrent) { }

        ActionType type;
        QList<mvdid> itemIds;
        QList<QTreeWidgetItem *> items;
        bool current;
    };

    MvdSDTreeWidget(QWidget *parent = 0);
    MvdSDTreeWidget(Movida::DataRole ds, const MvdMovie &movie,
    MvdMovieCollection *c, QWidget *parent = 0);

    void setDataSource(Movida::DataRole ds);
    Movida::DataRole dataSource() const { return mDS; }

    void resetToDefaults();

    void setMovie(const MvdMovie &m);
    void store(MvdMovie &m);

    void setMovieCollection(MvdMovieCollection *mc);
    MvdMovieCollection *movieCollection() const { return mCollection; }

    QList<quint32> currentValues(quint32 excluded = 0, bool excludeNewItems = true) const;

    bool isModified() const;

protected:
    void startDrag(Qt::DropActions supportedActions);
    void dragMoveEvent(QDragMoveEvent *event);

signals:
    void modifiedStatusChanged(bool modified);

private slots:
    void showContextMenu(QTreeWidgetItem *item, int col);
    void updatedModifiedStatus();

private:
    MvdMovie mMovie;
    MvdMovieCollection *mCollection;
    Movida::DataRole mDS;
    bool mModified;

    void init();
    void setPersonRoleData(const QList<MvdRoleItem> &d);
    void setSimpleData(const QList<mvdid> &d);

    inline QMenu *createItemMenu(const QString &label,
    const QMap<QString, ActionDescriptor> &actions, ActionType type);

    inline QString joinStringList(const QStringList &list, const QString &sep,
    const QString &def = QString()) const;
    inline QStringList splitString(const QString &s) const;

    inline void executeAction(ActionDescriptor ad, const QVariant &data = QVariant());

    inline QMap<QString, ActionDescriptor> generateActions(quint32 selected = 0, int *itemCount = 0, int max = -1);
    inline void generateActions(const QHash<mvdid, MvdSdItem> &d,
    const QList<quint32> &current, QMap<QString, ActionDescriptor> &actions,
    quint32 selected = 0);

    inline void setModified(bool m);

    inline MvdTreeWidgetItem *appendPlaceHolder();
    inline void removePlaceHolder(QTreeWidgetItem *item);

    void keyPressEvent(QKeyEvent *event);

    friend class MvdSDDelegate;
};
Q_DECLARE_METATYPE(MvdSDTreeWidget::ActionDescriptor)

class MvdSDDelegate :
    public QItemDelegate
{
Q_OBJECT

public:
    MvdSDDelegate(MvdSDTreeWidget *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
    const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
    const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
    const QModelIndex &index) const;

    bool eventFilter(QObject *object, QEvent *event);

private:
    enum ValidatorUse { ValidationUse, MaskUse };

    inline MvdSDTreeWidget *tree() const;
    inline Movida::ItemValidator validatorType(const QModelIndex &index,
    const MvdSDTreeWidget &tree, QVariant *data = 0, ValidatorUse use = ValidationUse) const;
    inline bool isItemValid(Movida::DataRole ds,
    const QTreeWidgetItem &item) const;
};

#endif // MVD_SDTREEWIDGET_H
