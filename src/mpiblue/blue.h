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

class QTemporaryFile;
class QTextStream;

class MpiBlue : public MvdPluginInterface
{
	Q_OBJECT

public:
	enum UpdateInterval {
		UpdateAlways,
		UpdateOnce,
		UpdateDaily,
		UpdateWeekly,
		UpdateCustom
	};
	
	enum ScriptStatus {
		InvalidScript= 0,
		ValidScript,
		NoUpdatedScript
	};

	struct Engine
	{
		inline Engine() : 
			scriptsFetched(false), 
			updateInterval(UpdateOnce), 
			updateIntervalHours(0)
		{}

		inline Engine(const QString& n) : 
			name(n), 
			scriptsFetched(false), 
			updateInterval(UpdateOnce), 
			updateIntervalHours(0)
		{}

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
			updateInterval = o.updateInterval;
			updateIntervalHours = o.updateIntervalHours;
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

		UpdateInterval updateInterval;
		quint8 updateIntervalHours;
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

	static UpdateInterval updateIntervalFromString(QString s, quint8* hours);
	static QString updateIntervalToString(UpdateInterval i, quint8 hours);

	static bool engineRequiresUpdate(const Engine& engine);
	
	static void setScriptPaths(Engine* engine);
	static QString locateScriptPath(const QString& name);
	static MpiBlue::ScriptStatus isValidScriptFile(const QString& path);
	static MpiBlue::ScriptStatus isValidScriptFile(QTemporaryFile* tempFile, bool httpNotModified);

private:
	void loadEngines(bool loadBundled = true);
	void loadEnginesFromFile(const QString& path);
	inline bool isValidEngine(const Engine& engine) const;
		
	static MpiBlue::ScriptStatus isValidScriptFile(QTextStream& stream);

	QList<Engine*> mEngines;
};

namespace MpiBluePlugin {
	extern MpiBlue* instance;
};

extern "C" MPI_EXPORT_BLUE MvdPluginInterface* pluginInterface(QObject* parent);

#endif // MPI_BLUE_H
