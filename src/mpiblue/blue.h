/**************************************************************************
** Filename: blue.h
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

#ifndef MPI_BLUE_H
#define MPI_BLUE_H

#include "blueglobal.h"
#include "plugininterface.h"

class MpiBlue : public MvdPluginInterface
{
	Q_OBJECT

public:
	struct Engine
	{
		inline Engine() : scriptsFetched(false) {}
		inline Engine(const QString& n) : name(n), scriptsFetched(false) {}
		inline Engine(const Engine& o)
		{
			name = o.name;
			displayName = o.displayName;
			updateUrl = o.updateUrl;
			interpreter = o.interpreter;
			resultsScript = o.resultsScript;
			resultsUrl = o.resultsUrl;
			importScript = o.importScript;
			importUrl = o.importUrl;
			searchUrl = o.searchUrl;
			scriptsFetched = o.scriptsFetched;
		}

		inline bool operator<(const Engine& o) const
		{
			return displayName < o.displayName;
		}

		QString name;
		QString displayName;
		QString updateUrl;

		QString interpreter;
		
		QString resultsScript;
		QString resultsUrl;

		QString importScript;
		QString importUrl;

		QString searchUrl;

		bool scriptsFetched;
	};

	MpiBlue(QObject* parent = 0);
	virtual ~MpiBlue();

	// MvdPluginInterface overloads:
	bool init();
	void unload();
	QString lastError() const;
	PluginInfo info() const;
	QList<PluginAction> actions() const;
	void actionTriggeredImplementation(const QString& name);

private:
	void loadEngines(bool loadBundled = true);
	void loadEnginesFromFile(const QString& path);
	inline bool isValidEngine(const Engine& engine) const;

	QList<Engine*> mEngines;
};

namespace MpiBluePlugin {
	static MpiBlue* instance = 0;
};

extern "C" MPI_EXPORT_BLUE MvdPluginInterface* pluginInterface(QObject* parent);

#endif // MPI_BLUE_H
