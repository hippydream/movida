/**************************************************************************
** Filename: completer.h
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

#ifndef MVD_COMPLETER_H
#define MVD_COMPLETER_H

#include "sharedglobal.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QAbstractProxyModel>
#include <QtGui/QItemDelegate>

class MvdCompletionEngine;
class MvdCompletionModel;
class QAbstractItemView;
class QWidget;

class MVD_EXPORT_SHARED MvdCompleter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString completionPrefix READ completionPrefix WRITE setCompletionPrefix)
    Q_PROPERTY(ModelSorting modelSorting READ modelSorting WRITE setModelSorting)
    Q_PROPERTY(CompletionMode completionMode READ completionMode WRITE setCompletionMode)
    Q_PROPERTY(int completionColumn READ completionColumn WRITE setCompletionColumn)
    Q_PROPERTY(int completionRole READ completionRole WRITE setCompletionRole)
    Q_PROPERTY(Qt::CaseSensitivity caseSensitivity READ caseSensitivity WRITE setCaseSensitivity)
    Q_PROPERTY(bool wrapAround READ wrapAround WRITE setWrapAround)

public:
    enum CompletionMode {
        PopupCompletion,
        UnfilteredPopupCompletion,
        InlineCompletion
    };

    enum ModelSorting {
        UnsortedModel = 0,
        CaseSensitivelySortedModel,
        CaseInsensitivelySortedModel
    };

    MvdCompleter(QObject *parent = 0);
    MvdCompleter(QAbstractItemModel *model, QObject *parent = 0);
    MvdCompleter(const QStringList& completions, QObject *parent = 0);
    virtual ~MvdCompleter();

    void setWidget(QWidget *widget);
    QWidget *widget() const;

    void setModel(QAbstractItemModel *c);
    QAbstractItemModel *model() const;

    void setCompletionMode(CompletionMode mode);
    CompletionMode completionMode() const;

    QAbstractItemView *popup() const;
    void setPopup(QAbstractItemView *popup);

    void setCaseSensitivity(Qt::CaseSensitivity caseSensitivity);
    Qt::CaseSensitivity caseSensitivity() const;

    void setModelSorting(ModelSorting sorting);
    ModelSorting modelSorting() const;

    void setCompletionColumn(int column);
    int completionColumn() const;

    void setCompletionRole(int role);
    int completionRole() const;

    bool wrapAround() const;

    int completionCount() const;
    bool setCurrentRow(int row);
    int currentRow() const;

    QModelIndex currentIndex() const;
    QString currentCompletion() const;

    MvdCompletionModel *completionModel() const;

    QString completionPrefix() const;

public slots:
    void setCompletionPrefix(const QString &prefix);
    void complete(const QRect& rect = QRect());
    void setWrapAround(bool wrap);

public:
    virtual QString pathFromIndex(const QModelIndex &index) const;
    virtual QStringList splitPath(const QString &path) const;

protected:
    bool eventFilter(QObject *o, QEvent *e);
    bool event(QEvent *);

signals:
    void activated(const QString &text);
    void activated(const QModelIndex &index);
    void highlighted(const QString &text);
    void highlighted(const QModelIndex &index);

private:
    Q_DISABLE_COPY(MvdCompleter)

    class Private;
    friend class Private;
    Private *d;
};

//////////////////////////////////////////////////////////////////////

class MvdIndexMapper
{
public:
    MvdIndexMapper() : v(false), f(0), t(-1) { }
    MvdIndexMapper(int f, int t) : v(false), f(f), t(t) { }
    MvdIndexMapper(QVector<int> vec) : v(true), vector(vec), f(-1), t(-1) { }

    inline int count() const { return v ? vector.count() : t - f + 1; }
    inline int operator[] (int index) const { return v ? vector[index] : f + index; }
    inline int indexOf(int x) const { return v ? vector.indexOf(x) : ((t < f) ? -1 : x - f); }
    inline bool isValid() const { return !isEmpty(); }
    inline bool isEmpty() const { return v ? vector.isEmpty() : (t < f); }
    inline void append(int x) { Q_ASSERT(v); vector.append(x); }
    inline int first() const { return v ? vector.first() : f; }
    inline int last() const { return v ? vector.last() : t; }
    inline int from() const { Q_ASSERT(!v); return f; }
    inline int to() const { Q_ASSERT(!v); return t; }
    inline int cost() const { return vector.count()+2; }

private:
    bool v;
    QVector<int> vector;
    int f, t;
};

//////////////////////////////////////////////////////////////////////

class MvdMatchData
{
public:
    MvdMatchData() : exactMatchIndex(-1) { }
    MvdMatchData(const MvdIndexMapper& indices, int em, bool p) :
        indices(indices), exactMatchIndex(em), partial(p) { }

    inline bool isValid() const { return indices.isValid(); }

    MvdIndexMapper indices;
    int  exactMatchIndex;
    bool partial;
};

//////////////////////////////////////////////////////////////////////

class MvdCompletionEngine : public QObject
{
    Q_OBJECT

public:
    typedef QMap<QString, MvdMatchData> CacheItem;
    typedef QMap<QModelIndex, CacheItem> Cache;

    MvdCompletionEngine(MvdCompleter *c);
    virtual ~MvdCompletionEngine();

    MvdCompleter *completer() const;

    void filter(const QStringList &parts);

    MvdMatchData filterHistory();
    bool matchHint(QString, const QModelIndex&, MvdMatchData*);

    void saveInCache(QString, const QModelIndex&, const MvdMatchData&);
    bool lookupCache(QString part, const QModelIndex& parent, MvdMatchData *m);

    virtual void filterOnDemand(int) { }
    virtual MvdMatchData filter(const QString&, const QModelIndex&, int) = 0;

    int currentRow() const;
    void setCurrentRow(int n);

    int currentPartsCount() const;
    QStringList currentParts() const;

    void clearCache();

    QModelIndex currentParent() const;

    int matchCount() const;
    MvdMatchData& currentMatch() const;
    MvdMatchData& historyMatch() const;

protected:
    Cache& cache() const;

private:
    class Private;
    Private *d;
};

//////////////////////////////////////////////////////////////////////

class MvdSortedModelEngine : public MvdCompletionEngine
{
public:
    MvdSortedModelEngine(MvdCompleter *c) : MvdCompletionEngine(c) { }
    MvdMatchData filter(const QString&, const QModelIndex&, int);
    MvdIndexMapper indexHint(QString, const QModelIndex&, Qt::SortOrder);
    Qt::SortOrder sortOrder(const QModelIndex&) const;
};

//////////////////////////////////////////////////////////////////////

class MvdUnsortedModelEngine : public MvdCompletionEngine
{
public:
    MvdUnsortedModelEngine(MvdCompleter *c) : MvdCompletionEngine(c) { }

    void filterOnDemand(int);
    MvdMatchData filter(const QString&, const QModelIndex&, int);

private:
    int buildIndices(const QString& str, const QModelIndex& parent, int n,
        const MvdIndexMapper& iv, MvdMatchData* m);
};

//////////////////////////////////////////////////////////////////////

class MvdCompletionModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    MvdCompletionModel(MvdCompleter *c, QObject *parent);
    virtual ~MvdCompletionModel();

    void filter(const QStringList& parts);
    int completionCount() const;
    int currentRow() const;
    bool setCurrentRow(int row);
    QModelIndex currentIndex(bool) const;
    void resetModel();

    QModelIndex index(int row, int column, const QModelIndex & = QModelIndex()) const;
    int rowCount(const QModelIndex &index = QModelIndex()) const;
    int columnCount(const QModelIndex &index = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex & = QModelIndex()) const { return QModelIndex(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    void setSourceModel(QAbstractItemModel *sourceModel);
    QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
    QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;

    MvdCompleter *completer() const;
    MvdCompletionEngine *engine() const;

    void setFiltered(bool);
    bool filtered() const { return !showAll(); }
    void setShowAll(bool b) { setFiltered(!b); }
    bool showAll() const;

    void updateEngine();

signals:
    void rowsAdded();

public slots:
    void invalidate();
    void rowsInserted();
    void modelDestroyed();

private:
    class Private;
    Private *d;
};

//////////////////////////////////////////////////////////////////////

class MvdCompleterItemDelegate : public QItemDelegate
{
public:
    MvdCompleterItemDelegate(QAbstractItemView *view);

    void paint(QPainter *p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const;

private:
    QAbstractItemView *mView;
};

#endif // MVD_COMPLETER_H
