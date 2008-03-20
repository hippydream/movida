/**************************************************************************
** Filename: plugininterface.cpp
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

#include "plugininterface.h"

class MvdPluginInterface_P
{
public:
	QString userDataStore;
	QString globalDataStore;
};

MvdPluginInterface::MvdPluginInterface(QObject* parent)
	: QObject(parent), d(new MvdPluginInterface_P)
{
}

MvdPluginInterface::~MvdPluginInterface()
{
	delete d;
}

void MvdPluginInterface::actionTriggered(const QString& name)
{
	actionTriggeredImplementation(name);
}

//! Returns the data store path for this plugin. The directory is ensured to exist.
QString MvdPluginInterface::dataStore(Movida::Scope scope) const
{
	return scope == Movida::UserScope ? d->userDataStore : d->globalDataStore;
}

/*!
	Sets the path of the directory where plugin-specific data can be stored.
	Do not set this value from the plugin. Movida will initialize it and
	create the appropriate directories if necessary.

	Scope specifies whether the data store is in the user's plug-ins directory
	or in the global plug-in directory. Please refer to MvdPathResolver or to
	the developer docs for the exact location of these directories on each platform.
*/
void MvdPluginInterface::setDataStore(const QString& path, Movida::Scope scope)
{
	if (scope == Movida::UserScope)
		d->userDataStore = path;
	else d->globalDataStore = path;
}
