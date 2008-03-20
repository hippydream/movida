/**************************************************************************
** Filename: shareddata.h
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

#ifndef MVD_SHAREDDATA_H
#define MVD_SHAREDDATA_H

#include "global.h"

#include <QObject>
#include <QHash>
#include <QList>
#include <QString>
#include <QLocale>

class MvdSharedData;
class MvdSharedData_P;
class MvdSdItem;

class MVD_EXPORT MvdSharedData : public QObject
{
	Q_OBJECT

public:
	typedef QHash<mvdid, MvdSdItem> ItemList;

	enum ReferenceType
	{
		MovieReferences = 0x01,
		PersonReferences = 0x02,
		AllReferences = MovieReferences | PersonReferences
	};
	Q_DECLARE_FLAGS(ReferenceTypes, ReferenceType)

	enum ReferenceChange { AddReference, RemoveReference };

	static Movida::DataRole roleFromString(QString role);
	static QString roleToString(Movida::DataRole role);

	MvdSharedData();
	MvdSharedData(const MvdSharedData& s);
	~MvdSharedData();
	MvdSharedData& operator=(const MvdSharedData& m);

	// Item lookup
	//! \todo Add fuzzy item search
	MvdSdItem item(mvdid id) const;
	ItemList items(Movida::DataRole role) const;
	mvdid findItem(const MvdSdItem& item) const;
	mvdid findItemByValue(QString value, Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

	// Misc
	int usageCount(mvdid id, ReferenceTypes rt = AllReferences) const;

	// Add/edit
	mvdid addItem(const MvdSdItem& item);
	bool updateItem(mvdid id, const MvdSdItem& item);

	// References
	void addMovieLink(mvdid id, mvdid movie);
	void removeMovieLink(mvdid id, mvdid movie);
	void addPersonLink(mvdid id, mvdid person);
	void removePersonLink(mvdid id, mvdid person);

	// Remove
	bool removeItem(mvdid id);

	// Convenience method, returns the number of items in the SD
	int countItems(Movida::DataRole type = Movida::NoRole) const;

	// Utility methods
	void setAutoPurge(bool enable);
	bool autoPurge() const;
	bool canPurge() const;

public slots:
	void clear();
	bool purgeData();

signals:
	void itemAdded(mvdid id);
	void itemRemoved(mvdid id);
	void itemUpdated(mvdid id);
	void itemReferenceChanged(mvdid id);

	void cleared();

private:
	MvdSharedData_P* d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdSharedData::ReferenceTypes)

#endif // MVD_SHAREDDATA_H
