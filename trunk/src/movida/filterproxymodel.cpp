/**************************************************************************
** Filename: filterproxymodel.cpp
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

#include "filterproxymodel.h"
#include "guiglobal.h"
#include "collectionmodel.h"
#include "mvdcore/movie.h"
#include "mvdcore/shareddata.h"
#include "mvdcore/moviecollection.h"

//! Creates a new filter proxy that defaults to a quick search on the movie title attribute (localized and original).
MvdFilterProxyModel::MvdFilterProxyModel(QObject* parent)
: QSortFilterProxyModel(parent), mInvalidQuery(false)
{
	mMovieAttributes << Movida::TitleAttribute << Movida::OriginalTitleAttribute 
		<< Movida::DirectorsAttribute << Movida::CastAttribute // Producers are not so relevant - avoid bloating search results
		<< Movida::YearAttribute // Numeric values - won't bloat search results
		<< Movida::TagsAttribute;
	mSortColumn = (int) Movida::TitleAttribute;
	mSortOrder = Qt::AscendingOrder;
}

void MvdFilterProxyModel::setQuickFilterAttributes(const QByteArray& alist)
{
	mMovieAttributes.clear();
	for (int i = 0; i < alist.size(); ++i)
		mMovieAttributes << (Movida::MovieAttribute)alist.at(i);
}

QByteArray MvdFilterProxyModel::quickFilterAttributes() const
{
	QByteArray ba(mMovieAttributes.size(), '\0');

	for (int i = 0; i < mMovieAttributes.size(); ++i)
		ba[i] = (const char)mMovieAttributes.at(i);
	return ba;
}

bool MvdFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	if (mInvalidQuery)
		return false;

	Qt::CaseSensitivity cs = filterCaseSensitivity();

	foreach (QString s, mPlainStrings) {
		bool match = false;
		for (int i = 0; i < mMovieAttributes.size() && !match; ++i) {
			QModelIndex index = sourceModel()->index(sourceRow, (int)mMovieAttributes.at(i), sourceParent);
			match = sourceModel()->data(index).toString().contains(s, cs);
		}

		if (!match)
			return false;
	}

	foreach (Function f, mFunctions) {
		bool match = false;
		match = testFunction(sourceRow, sourceParent, f);
		if (!match)
			return false;
	}

	return true;
}

//! Convenience method only.
void MvdFilterProxyModel::sortByAttribute(Movida::MovieAttribute attr, Qt::SortOrder order)
{
	sort((int) attr, order);
}

void MvdFilterProxyModel::sort(int column, Qt::SortOrder order)
{
	mSortColumn = column;
	mSortOrder = order;
	QSortFilterProxyModel::sort(column, order);
	emit sorted();
}

int MvdFilterProxyModel::sortColumn() const
{
	return mSortColumn;
}

Movida::MovieAttribute MvdFilterProxyModel::sortAttribute() const
{
	return (Movida::MovieAttribute) sortColumn();
}

Qt::SortOrder MvdFilterProxyModel::sortOrder() const
{
	return mSortOrder;
}

bool MvdFilterProxyModel::testFunction(int sourceRow, const QModelIndex& sourceParent, 
	const Function& function) const
{
	switch (function.type) {
	case Movida::MovieIdFilter: {
		QList<mvdid> ids = idList(function.parameters);
		if (ids.isEmpty()) return false;

		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::TitleAttribute, sourceParent);
		mvdid currentId = index.data(Movida::IdRole).toUInt();
		
		return ids.contains(currentId) != function.neg;

	} break;
	case Movida::SharedDataIdFilter: {
		QList<mvdid> ids = idList(function.parameters);
		if (ids.isEmpty()) return false;

		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::TitleAttribute, sourceParent);
		mvdid currentMovieId = index.data(Movida::IdRole).toUInt();

		MvdCollectionModel* m = dynamic_cast<MvdCollectionModel*>(this->sourceModel());
		Q_ASSERT(m);

		const MvdMovieCollection* c = m->movieCollection();
		if (!c) return false;

		MvdMovie mov = c->movie(currentMovieId);
		QString x = mov.validTitle();

		MvdSharedData& sd = c->sharedData();
		
		bool match = true;
		foreach (mvdid id, ids) {
			MvdSdItem item = sd.item(id);
			match = item.movies.contains(currentMovieId);
			if (!match) break;
		}
		
		return match != function.neg;

	} break;
	case Movida::MarkAsSeenFilter: {
		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::SeenAttribute, sourceParent);
		bool seen = index.data().toBool();
		return seen != function.neg;
		
	} break;
	case Movida::MarkAsLoanedFilter: {
		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::LoanedAttribute, sourceParent);
		bool loaned = index.data().toBool();
		return loaned != function.neg;
		
	} break;
	case Movida::MarkAsSpecialFilter: {
		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::SpecialAttribute, sourceParent);
		bool spec = index.data().toBool();
		return spec != function.neg;
		
	} break;
	case Movida::RatingFilter: {
		if (function.parameters.isEmpty())
			return false;

		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::RatingAttribute, sourceParent);
		int rating = index.data(Movida::RawDataRole).toInt();

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
	case Movida::RunningTimeFilter: {
		if (function.parameters.isEmpty())
			return false;

		QModelIndex index = sourceModel()->index(sourceRow, (int)Movida::RunningTimeAttribute, sourceParent);
		int rt = index.data(Movida::RawDataRole).toUInt();
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
			foreach (QString cap, rxCaptures)
				if (!cap.isEmpty())
					captures.append(cap);

			if (captures.size() == 1) {
				QString s = captures.at(0);
				if (s.isEmpty())
					return false;
				if (s.at(0).isNumber())
					min = s.toInt() * 60; // hours
				else return false; // only <>= operator
			}
			else if (captures.size() == 2) {
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
	default: ;
	}

	return false;
}

//! Returns true if filter is empty or does not contain invalid filter functions.
bool MvdFilterProxyModel::rebuildPatterns()
{
	mFunctions.clear();
	mPlainStrings.clear();
	mInvalidQuery = false;

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
		if (!s.isEmpty() && !mPlainStrings.contains(s))
			mPlainStrings.append(s);
	}

	QString s = mQuery.right(mQuery.length() - offset).trimmed();
	if (!s.isEmpty() && !mPlainStrings.contains(s)) mPlainStrings.append(s);

	return true;
}

/*!
	Sets a query string to be used for advanced pattern matching.
	
	The string can contain both plain text strings and filter functions
	of the form @FUNCTION_NAME(PARAMETERS). For a list of supported functions,
	please refer to the Movida::FilterFunction enum.

	Returns false if the string contains invalid filter functions.
	\todo Check function parameters.
*/
bool MvdFilterProxyModel::setFilterAdvancedString(const QString& q)
{
	mQuery = q.trimmed();
	bool res = rebuildPatterns();
	invalidateFilter();
	return res;
}

//! Returns the current string used for advanced pattern matching.
QString MvdFilterProxyModel::filterAdvancedString() const
{
	return mQuery;
}

//! Convenience method, converts a list of string IDs to mvdids.
QList<mvdid> MvdFilterProxyModel::idList(const QStringList& sl) const
{
	QList<mvdid> ids;
	bool ok;
	foreach (QString s, sl) {
		mvdid id = s.toUInt(&ok);
		if (ok) ids << id;
	}
	return ids;
}
