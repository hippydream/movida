/**************************************************************************
** Filename: settings.h
**
** Copyright (C) 2007 Angius Fabrizio. All rights reserved.
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

#ifndef MVD_SETTINGS_H
#define MVD_SETTINGS_H

#include "global.h"
#include <QVariant>

class MVD_EXPORT MvdSettings
{
public:
	enum Status
	{
		NoError = 0,
		AccessError,
		FormatError
	};

	static MvdSettings& instance();
	virtual ~MvdSettings();

	void clear();

	void beginGroup(const QString& prefix);
	void endGroup();
	QString group() const;

	int beginReadArray(const QString& prefix);
	void beginWriteArray(const QString& prefix, int size = -1);
	void setArrayIndex(int i);
	void endArray();

	bool contains(const QString& key) const;

	void remove(const QString& key);
	void setDefaultValue(const QString& key, const QVariant& value);
	void setValue(const QString& key, const QVariant& value);
	QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;

	Status status() const;

private:
	static MvdSettings* mInstance;
	MvdSettings();
};

namespace Movida
{
	extern MVD_EXPORT MvdSettings& settings();
}

#endif // MVD_SETTINGS_H
