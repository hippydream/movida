/**************************************************************************
** Filename: exportengine.h
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

#ifndef MVD_EXPORTENGINE_H
#define MVD_EXPORTENGINE_H

#include "sharedglobal.h"
#include <QString>
#include <QUrl>

/*!
	\class MvdExportEngine exportengine.h
	\ingroup MovidaShared

	\brief Describes a local/network export engine.
*/

class MvdExportEngine
{
public:
	enum EngineOption {
		NoEngineOption = 0,
		CustomizableAttributesOption = 1
	};
	Q_DECLARE_FLAGS(EngineOptions, EngineOption);

	//! Creates a new search engine.
	MvdExportEngine() : canConfigure(false) {}

	//! Creates a new search engine with given name.
	MvdExportEngine(const QString& displayName)
	: name(displayName), canConfigure(false)
	{ }

	bool operator ==(const MvdExportEngine& o) const {
		return !QString::compare(name, o.name, Qt::CaseInsensitive);
	}

	QString name;
	QString urlFilter;
	EngineOptions options;

	bool canConfigure;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdExportEngine::EngineOptions)

#endif // MVD_EXPORTENGINE_H
