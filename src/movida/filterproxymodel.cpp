/**************************************************************************
** Filename: filterproxymodel.cpp
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

#include "filterproxymodel.h"

#include "collectionmodel.h"
#include "guiglobal.h"

#include "mvdcore/movie.h"
#include "mvdcore/moviecollection.h"
#include "mvdcore/shareddata.h"
#include "mvdcore/utils.h"

#include <QtCore/QCache>

class MvdFilterProxyModel::Private
{
public:
    Private(MvdFilterProxyModel *p) :
        mInvalidQuery(false),
        mPlainTextAppended(false),
        q(p)
    {

    }

    struct Function {
        Function(Movida::FilterFunction ff, const QStringList &p, bool negated) :
            type(ff),
            parameters(p),
            neg(negated)
        { }

        Movida::FilterFunction type;
        QStringList parameters;
        bool neg;
    };

    void addPlainTextQuery(const QString &s) {
        Q_ASSERT(!s.isEmpty());
        
        QString pattern;

        bool foundQuote = false;
        const QChar* uc_begin = s.unicode();
        const QChar* uc_end = uc_begin + s.length();
        const QChar* c = uc_begin;
        while (c != uc_end) {
            if (*c == '\"') {
                if (foundQuote) {
                    foundQuote = false;
                    if (!pattern.isEmpty()) {
                        if (!mPlainStrings.contains(pattern)) {
                            mPlainStrings.append(pattern);
                        }
                        pattern.clear();
                    }
                } else foundQuote = true;
            } else if (c->isSpace()) {
                if (foundQuote)
                    pattern.append(*c);
                else {
                    if (!pattern.isEmpty()) {
                        if (!mPlainStrings.contains(pattern)) {
                            mPlainStrings.append(pattern);
                        }
                        pattern.clear();
                    }
                }
            } else {
                pattern.append(*c);
            }
            ++c;
        }

        if (!pattern.isEmpty()) {
            if (foundQuote) {
                QStringList l = pattern.split(QRegExp("\\s"), QString::SkipEmptyParts);
                for (int i = 0; i < l.size(); ++i) {
                    QString s = l.at(i);
                    if (!mPlainStrings.contains(s)) {
                        mPlainStrings.append(s);
                    }
                }
            } else {
                if (!mPlainStrings.contains(pattern)) {
                    mPlainStrings.append(pattern);
                }
            }
        }
    }

    bool rebuildPatterns();
    void optimizeNewPatterns();

    bool plainTextFilter(mvdid id, int sourceRow);
    bool testFunction(int sourceRow, const QModelIndex &sourceParent,
        const Function &function) const;
    inline QList<mvdid> idList(const QStringList &sl) const;
    QString textForMovie(mvdid id, int sourceRow);

    QList<Movida::MovieAttribute> mMovieAttributes;
    int mSortColumn;
    Qt::SortOrder mSortOrder;

    QString mQuery;
    bool mInvalidQuery;
    QList<Function> mFunctions;
    QStringList mPlainStrings, mOldPlainStrings;

    QCache<mvdid, QString> mTextCache;

    // Optimization tricks
    bool mPlainTextAppended;
    QList<mvdid> mPlainTextFailedMatches;

private:
    MvdFilterProxyModel *q;
};

//////////////////////////////////////////////////////////////////////////


//! Creates a new filter proxy that defaults to a quick search on the movie title attribute (localized and original).
MvdFilterProxyModel::MvdFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    d(new Private(this))
{
    d->mMovieAttributes << Movida::TitleAttribute << Movida::OriginalTitleAttribute
                     << Movida::DirectorsAttribute << Movida::CastAttribute // Producers are not so relevant - avoid bloating search results
                     << Movida::YearAttribute // Numeric values - won't bloat search results
                     << Movida::TagsAttribute;
    d->mSortColumn = (int)Movida::TitleAttribute;
    d->mSortOrder = Qt::AscendingOrder;
}

MvdFilterProxyModel::~MvdFilterProxyModel()
{
    delete d;
}

void MvdFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    QAbstractItemModel* old_model = this->sourceModel();
    if (old_model) {
        disconnect(old_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(onSourceDataChanged(QModelIndex,QModelIndex)));
        disconnect(sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(onSourceRowsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(old_model, SIGNAL(destroyed(QObject*)),
            this, SLOT(onSourceModelDestroyed(QObject*)));
    }

    if (sourceModel) {
        connect(sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(onSourceDataChanged(QModelIndex,QModelIndex)));
        connect(sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(onSourceDataChanged(QModelIndex,QModelIndex)));
        connect(sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(onSourceRowsAboutToBeRemoved(QModelIndex,int,int)));
        connect(old_model, SIGNAL(destroyed(QObject*)),
            this, SLOT(onSourceModelDestroyed(QObject*)));
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);
}

void MvdFilterProxyModel::onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    int row = topLeft.row();
    int end = bottomRight.row();
    while (row >= 0 && row <= end) {
        QModelIndex index = sourceModel()->index(row, 0);
        bool ok;
        mvdid id = (mvdid) index.data(Movida::IdRole).toInt(&ok);
        if (ok && id != MvdNull) {
            d->mTextCache.remove(id);
            d->mPlainTextFailedMatches.removeOne(id);
        }
        ++row;
    }
}

void MvdFilterProxyModel::onSourceRowsAboutToBeRemoved(const QModelIndex &, int row, int end)
{
    while (row >= 0 && row <= end) {
        QModelIndex index = sourceModel()->index(row, 0);
        bool ok;
        mvdid id = (mvdid) index.data(Movida::IdRole).toInt(&ok);
        if (ok && id != MvdNull) {
            d->mTextCache.remove(id);
            d->mPlainTextFailedMatches.removeOne(id);
        }
        ++row;
    }
}

void MvdFilterProxyModel::onSourceModelDestroyed(QObject*)
{
    d->mTextCache.clear();
    d->mPlainTextFailedMatches.clear();
}

void MvdFilterProxyModel::setQuickFilterAttributes(const QByteArray &alist)
{
    d->mMovieAttributes.clear();
    for (int i = 0; i < alist.size(); ++i)
        d->mMovieAttributes << (Movida::MovieAttribute)alist.at(i);
    d->mTextCache.clear();
}

QByteArray MvdFilterProxyModel::quickFilterAttributes() const
{
    QByteArray ba(d->mMovieAttributes.size(), '\0');

    for (int i = 0; i < d->mMovieAttributes.size(); ++i)
        ba[i] = (const char)d->mMovieAttributes.at(i);
    return ba;
}

bool MvdFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (d->mInvalidQuery)
        return false;

    if (d->mPlainStrings.isEmpty() && d->mFunctions.isEmpty())
        return true;

    QModelIndex index = sourceModel()->index(sourceRow, 0);
    
    bool ok;
    mvdid id = (mvdid) index.data(Movida::IdRole).toInt(&ok);
    Q_ASSERT_X(ok, "MvdFilterProxyModel", "filterAcceptsRow(): invalid source row.");

    if (id == MvdNull)
        return false;
    
    QTime t;
    t.start();

    // Test plain text query parts
    bool accept = d->plainTextFilter(id, sourceRow);
    if (!accept) {
        return false;
    }

    // Test function parts
    QList<Private::Function>::ConstIterator fun_begin = d->mFunctions.constBegin();
    QList<Private::Function>::ConstIterator fun_end = d->mFunctions.constEnd();
    while (fun_begin != fun_end) {
        const Private::Function& fun = *fun_begin;
        bool match = d->testFunction(sourceRow, sourceParent, fun);
        if (!match) {
            return false;
        }
        ++fun_begin;
    }

    return true;
}

//! Convenience method only.
void MvdFilterProxyModel::sortByAttribute(Movida::MovieAttribute attr, Qt::SortOrder order)
{
    sort((int)attr, order);
}

void MvdFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    d->mSortColumn = column;
    d->mSortOrder = order;
    QSortFilterProxyModel::sort(column, order);
    emit sorted();
}

int MvdFilterProxyModel::sortColumn() const
{
    return d->mSortColumn;
}

Movida::MovieAttribute MvdFilterProxyModel::sortAttribute() const
{
    return (Movida::MovieAttribute)sortColumn();
}

Qt::SortOrder MvdFilterProxyModel::sortOrder() const
{
    return d->mSortOrder;
}

/*!
    Sets a query string to be used for advanced pattern matching.

    The string can contain both plain text strings and filter functions
    of the form @FUNCTION_NAME(PARAMETERS). For a list of supported functions,
    please refer to the Movida::FilterFunction enum.

    Returns false if the string contains invalid filter functions.
    \todo Check function parameters.
*/
bool MvdFilterProxyModel::setFilterAdvancedString(const QString &q)
{
    if (d->mQuery == q)
        return true;

    QTime t;
    t.start();
    d->mQuery = q.trimmed();
    bool res = d->rebuildPatterns();
    invalidateFilter();
    
    qDebug("setFilterAdvancedString() took %d ms for pattern %s", t.elapsed(), qPrintable(q));
    return res;
}

//! Returns the current string used for advanced pattern matching.
QString MvdFilterProxyModel::filterAdvancedString() const
{
    return d->mQuery;
}

bool MvdFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return QSortFilterProxyModel::lessThan(left, right);
}


//////////////////////////////////////////////////////////////////////////


bool MvdFilterProxyModel::Private::testFunction(int sourceRow, const QModelIndex &sourceParent,
    const Function &function) const
{
    switch (function.type) {
        case Movida::MovieIdFilter:
            {
                QList<mvdid> ids = idList(function.parameters);
                if (ids.isEmpty()) return false;

                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::TitleAttribute, sourceParent);
                mvdid currentId = index.data(Movida::IdRole).toUInt();

                return ids.contains(currentId) != function.neg;

            } break;

        case Movida::SharedDataIdFilter:
            {
                QList<mvdid> ids = idList(function.parameters);
                if (ids.isEmpty()) return false;

                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::TitleAttribute, sourceParent);
                mvdid currentMovieId = index.data(Movida::IdRole).toUInt();

                MvdCollectionModel *m = dynamic_cast<MvdCollectionModel *>(q->sourceModel());
                Q_ASSERT(m);

                const MvdMovieCollection *c = m->movieCollection();
                if (!c) return false;

                MvdMovie mov = c->movie(currentMovieId);
                QString x = mov.validTitle();

                MvdSharedData &sd = c->sharedData();

                bool match = true;
                foreach(mvdid id, ids)
                {
                    MvdSdItem item = sd.item(id);

                    match = item.movies.contains(currentMovieId);
                    if (!match) break;
                }

                return match != function.neg;

            } break;

        case Movida::MarkAsSeenFilter:
            {
                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::SeenAttribute, sourceParent);
                bool seen = index.data().toBool();
                return seen != function.neg;

            } break;

        case Movida::MarkAsLoanedFilter:
            {
                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::LoanedAttribute, sourceParent);
                bool loaned = index.data().toBool();
                return loaned != function.neg;

            } break;

        case Movida::MarkAsSpecialFilter:
            {
                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::SpecialAttribute, sourceParent);
                bool spec = index.data().toBool();
                return spec != function.neg;

            } break;

        case Movida::RatingFilter:
            {
                if (function.parameters.isEmpty())
                    return false;

                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::RatingAttribute, sourceParent);
                int rating = index.data(Movida::SortRole).toInt();

                QString x = function.parameters.first();
                QRegExp rx("\\s*([<>=]?)\\s*(\\d)\\s*");
                if (rx.indexIn(x) < 0)
                    return false;

                enum { Lt, Gt, Eq } op = Eq;
                int n;

                if (rx.numCaptures() == 2) {
                    op = rx.cap(1) == QLatin1String("<") ? Lt : rx.cap(1) == QLatin1String(">") ? Gt : Eq;
                    n = rx.cap(2).toInt();
                } else n = rx.cap(1).toInt();

                bool match;
                if (op == Lt)
                    match = rating < n;
                else if (op == Gt)
                    match = rating > n;
                else
                    match = rating == n;

                return match != function.neg;

            } break;

        case Movida::RunningTimeFilter:
            {
                if (function.parameters.isEmpty())
                    return false;

                QModelIndex index = q->sourceModel()->index(sourceRow, (int)Movida::RunningTimeAttribute, sourceParent);
                int rt = index.data(Movida::SortRole).toUInt();
                int min = -1;
                QString op;

                QString s = function.parameters.first();
                QRegExp rx("\\s*([<>=])?(\\d{1,3})\\s*m?\\s*");
                if (rx.exactMatch(s)) {
                    if (rx.numCaptures() == 2) {
                        op = rx.cap(1);
                        min = rx.cap(2).toInt();
                    }
                } else {
                    QString hm("(\\d{1,3})\\s*h(?:\\s*(\\d+)\\s*m?)?"); // 1h 20m OR 1h 20 OR 1h
                    QString qm("(\\d{1,3})\\s*'(?:\\s*(\\d+)\\s*(?:'')?)?"); // 1' 10'' OR 1' 10
                    QString dm("(\\d{1})[\\.:](\\d{1,2})"); // 1.20 or 1:20
                    rx.setPattern(QString("\\s*([<>=])?\\s*(?:(?:%1)|(?:%2)|(?:%3))\\s*")
                        .arg(hm).arg(qm).arg(dm)
                        );
                    if (!rx.exactMatch(s))
                        return false;
                    QStringList captures;
                    QStringList rxCaptures = rx.capturedTexts();
                    rxCaptures.removeAt(0); // First item is the whole match
                    foreach(QString cap, rxCaptures)
                        if (!cap.isEmpty())
                            captures.append(cap);

                    if (captures.size() == 1) {
                        QString s = captures.at(0);
                        if (s.isEmpty())
                            return false;
                        if (s.at(0).isNumber())
                            min = s.toInt() * 60; // hours
                        else return false; // only <>= operator
                    } else if (captures.size() == 2) {
                        QString s = captures.at(0);
                        if (s.isEmpty())
                            return false;
                        if (s.at(0).isNumber()) {
                            min = captures.at(0).toInt() * 60; // hours
                            min += captures.at(1).toInt(); // minutes
                        } else {
                            op = s;
                            min = captures.at(1).toInt() * 60; // hours
                        }
                    } else if (captures.size() == 3) {
                        op = captures.at(0);
                        min = captures.at(1).toInt() * 60; // hours
                        min += captures.at(2).toInt(); // minutes
                    }
                }

                if (min < 0)
                    return false;


                enum { Lt, Gt, Eq } _op = Eq;
                _op = op == QLatin1String("<") ? Lt : rx.cap(1) == QLatin1String(">") ? Gt : Eq;

                bool match;
                if (_op == Lt)
                    match = rt < min;
                else if (_op == Gt)
                    match = rt > min;
                else
                    match = rt == min;

                return match != function.neg;

            } break;

        default:
            ;
    }

    return false;
}

//! Returns true if filter is empty or does not contain invalid filter functions.
bool MvdFilterProxyModel::Private::rebuildPatterns()
{
    mFunctions.clear();
    mOldPlainStrings = mPlainStrings;
    mPlainStrings.clear();
    mInvalidQuery = false;
    mPlainTextAppended = false;

    if (mQuery.isEmpty())
        return true;

    // \B@([a-z_-\\.]+)\((.*)\)\B
    QRegExp frx("\\B([@!])([a-z_-\\\\.]+)\\((.*)\\)\\B");
    frx.setMinimal(true);
    frx.setCaseSensitivity(Qt::CaseInsensitive);

    int offset = 0;
    while (frx.indexIn(mQuery, offset) >= 0) {
        QString op = frx.cap(1);

        QString n = frx.cap(2);
        Movida::FilterFunction ff = Movida::filterFunction(n);
        if (ff == Movida::InvalidFilterFunction) {
            mInvalidQuery = true;
            return false;
        }

        QString ps = frx.cap(3).trimmed();
        QRegExp prx = Movida::filterFunctionRegExp(ff);
        if ((!prx.isEmpty() && !prx.exactMatch(ps)) || (prx.isEmpty() && !ps.isEmpty())) {
            mInvalidQuery = true;
            return false;
        }

        QStringList p = ps.split(QLatin1Char(','), QString::SkipEmptyParts);
        mFunctions.append(Function(ff, p, op == QLatin1String("!")));

        QString s = mQuery.mid(offset, frx.pos() - offset);

        offset += frx.matchedLength() + s.length();

        s = s.trimmed();
        if (!s.isEmpty()) {
            addPlainTextQuery(s);
        }
    }

    QString s = mQuery.right(mQuery.length() - offset).trimmed();
    if (!s.isEmpty()) {
        addPlainTextQuery(s);
    }

    optimizeNewPatterns();
    return true;
}

//! Attempts to perform some optimization steps after the search patterns have changed.
void MvdFilterProxyModel::Private::optimizeNewPatterns()
{
    bool plainTextAppended = false;
    if (!mOldPlainStrings.isEmpty()) {
        plainTextAppended = true;
        QStringList::ConstIterator begin = mPlainStrings.constBegin();
        QStringList::ConstIterator end = mPlainStrings.constEnd();
        while (begin != end && plainTextAppended) {
            const QString& s = *begin;
            QStringList::ConstIterator old_begin = mOldPlainStrings.constBegin();
            QStringList::ConstIterator old_end = mOldPlainStrings.constEnd();
            while (old_begin != old_end && plainTextAppended) {
                QString old_s = *old_begin;
                plainTextAppended = s.startsWith(old_s);
                ++old_begin;
            }
            
            ++begin;
        }
    }

    mPlainTextAppended = plainTextAppended;
    if (!mPlainTextAppended)
        mPlainTextFailedMatches.clear();
}


//! Convenience method, converts a list of string IDs to mvdids.
QList<mvdid> MvdFilterProxyModel::Private::idList(const QStringList &sl) const
{
    QList<mvdid> ids;
    bool ok;

    QStringList::ConstIterator begin = sl.constBegin();
    QStringList::ConstIterator end = sl.constEnd();
    while (begin != end) {
        const QString& s = *begin;
        mvdid id = s.toUInt(&ok);
        if (ok)
            ids.append(id);
        ++begin;
    }
    return ids;
}

/*!
    Returns the text that has to be searched for the given movie.
    This method is used only for text searches (i.e. not for search functions).
    A cache contains the text that needs to be searched for a given movie and
    which has been built by using the attributes specified by setMovieAttributes().
*/
QString MvdFilterProxyModel::Private::textForMovie(mvdid id, int sourceRow)
{
    QString* cachedText = mTextCache.object(id);
    if (cachedText)
        return *cachedText;

    QString title;
    bool titleAdded = false;

    // Build and cache the text for the given movie
    QString text;
    QList<Movida::MovieAttribute>::ConstIterator att_begin = mMovieAttributes.constBegin();
    QList<Movida::MovieAttribute>::ConstIterator att_end = mMovieAttributes.constEnd();
    while (att_begin != att_end) {
        bool skip = false;
        Movida::MovieAttribute att = *att_begin;
        QModelIndex index = q->sourceModel()->index(sourceRow, (int)att, QModelIndex());
        QString att_text = q->sourceModel()->data(index).toString();
        if (!titleAdded && (att == Movida::TitleAttribute || att == Movida::OriginalTitleAttribute)) {
            titleAdded = true;
            title = att_text;
        } else if (titleAdded && (att == Movida::TitleAttribute || att == Movida::OriginalTitleAttribute)) {
            // Check duplicate titles to shorten the search string
            if (title == att_text)
                skip = true;
        }
        if (!skip)
            text.append(att_text).append(QChar(QChar::LineSeparator));
        ++att_begin;
    }

    Movida::normalize(text);
    mTextCache.insert(id, new QString(text));

    return text;
}

bool MvdFilterProxyModel::Private::plainTextFilter(mvdid id, int sourceRow)
{
    if (mPlainTextAppended) {
        // Look for the previous match
        if (mPlainTextFailedMatches.contains(id))
            return false;
    }

    //    Qt::CaseSensitivity cs = filterCaseSensitivity();

    QStringList::ConstIterator begin = mPlainStrings.constBegin();
    QStringList::ConstIterator end = mPlainStrings.constEnd();
    while (begin != end) {
        QString queryPattern = *begin;
        Movida::normalize(queryPattern);

        bool match = false;

        QString targetString = textForMovie(id, sourceRow);
        match = targetString.contains(queryPattern, q->filterCaseSensitivity());

        if (!match) {
            mPlainTextFailedMatches.append(id);
            return false;
        }

        ++begin;
    }

    return true;
}

