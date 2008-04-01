/**************************************************************************
** Filename: blue.cpp
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

#include "blue.h"
#include "movieimport.h"
#include "mvdcore/core.h"
#include "mvdcore/logger.h"
#include "mvdcore/settings.h"
#include "mvdcore/pathresolver.h"
#include <QDateTime>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <math.h>

using namespace Movida;

Q_DECLARE_METATYPE(MpiBlue::Engine*);

//! \internal
MpiBlue* MpiBluePlugin::instance = 0;

//! Public interface for this plugin.
MvdPluginInterface* pluginInterface(QObject* parent)
{
	if (!MpiBluePlugin::instance)
		MpiBluePlugin::instance = new MpiBlue(parent);
	return MpiBluePlugin::instance;
}

MpiBlue::MpiBlue(QObject* parent)
: MvdPluginInterface(parent)
{
	Q_UNUSED(qRegisterMetaType<MpiBlue::Engine*>());

	QHash<QString,QVariant> parameters;
	parameters.insert("plugins/blue/script-signature", "movida blue plugin script");
	parameters.insert("plugins/blue/http-date", "ddd, dd MMM yyyy");
	parameters.insert("plugins/blue/http-time", "hh:mm:ss UTC");
	MvdCore::registerParameters(parameters);
}

MpiBlue::~MpiBlue()
{
}

bool MpiBlue::init()
{
	settings().setDefaultValue("plugins/blue/disableBundledEngines", false);
	loadEngines();
	return true;
}

void MpiBlue::unload()
{
	if (!mTempDir.isEmpty()) {
		Movida::paths().removeDirectoryTree(mTempDir);
	}
}

QString MpiBlue::lastError() const
{
	return QString();
}

MvdPluginInterface::PluginInfo MpiBlue::info() const
{
	MvdPluginInterface::PluginInfo info;
	info.uniqueId = QLatin1String("org.42cows.movida.mpi.blue");
	info.name = tr("Blue plugin");
	info.description = tr("Basic import/export plugin");
	info.author = "Fabrizio Angius";
	info.version = "0.8";
	return info;
}

QList<MvdPluginInterface::PluginAction> MpiBlue::actions() const
{
	QList<MvdPluginInterface::PluginAction> list;
	
	MvdPluginInterface::PluginAction a;
	a.text = tr("IMDb movie import");
	a.helpText = tr("Import movies from the IMDb website.");
	a.name = "imdb-import";
	a.type = MvdPluginInterface::ImportAction;
	list << a;

	a.text = tr("Reload engines");
	a.helpText = tr("Reloads the custom engines.");
	a.name = "reload-engines";
	a.type = MvdPluginInterface::GenericAction;
	list << a;

	return list;
}

void MpiBlue::actionTriggeredImplementation(const QString& name)
{
	if (name == "imdb-import") {
		MpiMovieImport mi(parent() ? parent() : this);
		mi.runImdbImport(mEngines);
	} else if (name == "reload-engines") {
		loadEngines();
	}
}

void MpiBlue::loadEngines(bool loadBundled)
{
	//! \todo Problem: we need to check for updated scripts *and* set the absolute file path 
	// (script might be in user or in global directory). AND we would like to do a LAZY update! 
	// Problem is that we need to remember what script has been fetched in the plugin (i.e. HERE) 
	// and not in the MpiMovieImport class, which is created only when necessary.
	if (loadBundled && !settings().value("plugins/blue/disableBundledEngines").toBool())
		loadEnginesFromFile(":/xml/engines.xml");

	QString externalEngines = dataStore(Movida::SystemScope).append("/engines.xml");
	if (QFile::exists(externalEngines))
		loadEnginesFromFile(externalEngines);

	externalEngines = dataStore(Movida::UserScope).append("/engines.xml");
	if (QFile::exists(externalEngines))
		loadEnginesFromFile(externalEngines);

	qSort(mEngines);

	// Fix name clashes
	QString lastName;
	int index = 0;
	for (int i = 0; i < mEngines.size(); ++i)
	{
		Engine* e = mEngines.at(i);
		if (e->displayName == lastName)
			e->displayName.append(" (").append(QString::number(++index)).append(")");
		else
		{
			lastName = e->displayName;
			index = 0;
		}
	}
}

/*!
	Loads an engines.xml file.
*/
void MpiBlue::loadEnginesFromFile(const QString& path)
{
	iLog() << QString("MpiBlue: Loading engines from '%1'.").arg(path);

	xmlDocPtr doc = 0;

	if (path.startsWith(":/"))
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
		{
			eLog() << "MpiBlue: Failed to load engines";
			return;
		}

		QByteArray data = file.readAll();
		doc = xmlParseMemory(data.constData(), data.size());
	}
	else doc = xmlParseFile(path.toLatin1().constData());

	if (!doc)
	{
		eLog() << "MpiBlue: Failed to load engines";
		return;
	}

	xmlNodePtr node = xmlDocGetRootElement(doc);
	if (xmlStrcmp(node->name, (const xmlChar*) "mpi-blue-engines"))
	{
		eLog() << QString("MpiBlue: Not a valid engines.xml file.").append(path);
		xmlFree(node);
		return;
	}

	//! \todo Check version?

	node = node->xmlChildrenNode;
	while (node)
	{
		if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar*) "engine"))
		{
			node = node->next;
			continue;
		}

		xmlChar* attr = xmlGetProp(node, (const xmlChar*) "name");
		QString engineName;
		if (attr)
		{
			engineName = QString::fromLatin1((const char*) attr).trimmed();
			xmlFree(attr);
		}

		if (engineName.isEmpty())
		{
			wLog() << "MpiBlue: discarding unnamed engine.";
			node = node->next;
			continue;
		}

		engineName = engineName.toLower();

		Engine engine(engineName);
		xmlNodePtr engineNode = node->xmlChildrenNode;
		while (engineNode)
		{
			if (engineNode->type != XML_ELEMENT_NODE)
			{
				engineNode = engineNode->next;
				continue;
			}

			if (!xmlStrcmp(engineNode->name, (const xmlChar*) "display-name"))
				engine.displayName = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "update-url"))
				engine.updateUrl = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "interpreter"))
				engine.interpreter = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "results-script"))
			{
				engine.resultsScript = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
				xmlChar* attr = xmlGetProp(engineNode, (const xmlChar*)"update-name");
				if (attr)
				{
					engine.resultsUrl = QString::fromLatin1((const char*)attr);
					xmlFree(attr);
				}
			}
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "import-script"))
			{
				engine.importScript = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
				xmlChar* attr = xmlGetProp(engineNode, (const xmlChar*)"update-name");
				if (attr)
				{
					engine.importUrl = QString::fromLatin1((const char*)attr);
					xmlFree(attr);
				}
			}
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "search-url"))
				engine.searchUrl = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "update-interval")) {
				QString mode = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
				engine.updateInterval = MpiBlue::updateIntervalFromString(mode, &(engine.updateIntervalHours));
			}

			engineNode = engineNode->next;
		}

		if (engine.name.isEmpty())
			engine.name = tr("Unnamed engine");
		if (engine.importScript.isEmpty())
			engine.importScript = engine.resultsScript;
		if (engine.importUrl.isEmpty())
			engine.importUrl = engine.resultsUrl;
		if (engine.resultsScript.isEmpty())
			engine.resultsScript = engine.importScript;
		if (engine.resultsUrl.isEmpty())
			engine.resultsUrl = engine.importUrl;

		if (isValidEngine(engine))
		{
			bool engineAdded = false;
			bool engineReplaced = false;

			for (int i = 0; i < mEngines.size() && !engineAdded; ++i)
			{
				const Engine& e = *(mEngines.at(i));

				if (e.name == engineName)
				{
					delete mEngines.takeAt(i);
					mEngines.insert(i, new Engine(engine));
					engineAdded = engineReplaced = true;
				}
			}
			
			if (!engineAdded)
				mEngines.append(new Engine(engine));

			if (engineReplaced)
				iLog() << QString("MpiBlue: Engine '%1' replaced.").arg(engine.name);
			else iLog() << QString("MpiBlue: Engine '%1' added.").arg(engine.name);
		}
		else wLog() << QString("MpiBlue: Engine '%1' discarded.").arg(engineName);

		node = node->next;
	}

	xmlFreeDoc(doc);
}

//!
bool MpiBlue::isValidEngine(const Engine& engine) const
{
	if (MvdCore::locateApplication(engine.interpreter).isEmpty())
		return false;
	if (engine.searchUrl.isEmpty())
		return false;
	if (engine.importScript.isEmpty() || engine.resultsScript.isEmpty())
		return false;
	
	return true;
}

//! Parses an update interval as defined in an engines.xml file and possibly sets the hours value of a custom interval.
MpiBlue::UpdateInterval MpiBlue::updateIntervalFromString(QString s, quint8* hours)
{
	s = s.trimmed().toLower();
	if (s == "always")
		return UpdateAlways;
	else if (s == "once")
		return UpdateOnce;
	else if (s == "daily")
		return UpdateDaily;
	else if (s == "weekly")
		return UpdateWeekly;
	else {
		QRegExp rx("(\\d*)h");
		if (rx.exactMatch(s)) {
			int n = rx.cap(1).toInt();
			if (hours && n > 0) {
				*hours = n;
				return UpdateCustom;
			}
		}
	}

	return UpdateOnce;
}

//! Converts an update interval to a string to be used in an engines.xml file.
QString MpiBlue::updateIntervalToString(UpdateInterval i, quint8 hours)
{
	switch (i) {
	case UpdateAlways: return "always";
	case UpdateOnce: return "once";
	case UpdateDaily: return "daily";
	case UpdateWeekly: return "weekly";
	case UpdateCustom: return QString::number(hours).append("h");
	default: ;
	}

	return QLatin1String("once");
}

//! Returns true if an engine can be updated and requires an update.
bool MpiBlue::engineRequiresUpdate(const Engine& engine)
{
	if (!engine.updateUrl.startsWith("http://"))
		return false;
		
	if (locateScriptPath(engine.resultsScript).isEmpty() || locateScriptPath(engine.importScript).isEmpty()) {
		iLog() << "MpiBlue:: Missing scripts. Forcing an update from the Internet.";
		return true;
	}

	switch (engine.updateInterval) {
	case UpdateAlways:
	case UpdateOnce:
		iLog() << "MpiBlue:: Engine " << engine.name << " has update interval set to 'always' or 'once'. Update check required.";
		return true;
	case UpdateDaily:
	{
		QString s = Movida::settings().value(QString("plugins/blue/engines/%1/updated").arg(engine.name)).toString();
		QDateTime lastUpdateDt = s.isEmpty() ? QDateTime() : QDateTime::fromString(s, Qt::ISODate);
		if (!lastUpdateDt.isValid()) {
			iLog() << "MpiBlue:: Engine " << engine.name << " has daily update interval but no previous update. Update check required.";
			return true;
		}

		iLog() << "MpiBlue:: Engine " << engine.name << " has daily update interval and a previous update. Checking if an update is necessary.";

		QDate today = QDate::currentDate();
		QDate lastUpdate = lastUpdateDt.date();

		return today != lastUpdate;
	}
	case UpdateWeekly:
	{
		QString s = Movida::settings().value(QString("plugins/blue/engines/%1/updated").arg(engine.name)).toString();
		QDateTime lastUpdateDt = s.isEmpty() ? QDateTime() : QDateTime::fromString(s, Qt::ISODate);
		if (!lastUpdateDt.isValid()) {
			iLog() << "MpiBlue:: Engine " << engine.name << " has weekly update interval but no previous update. Update check required.";
			return true;
		}

		iLog() << "MpiBlue:: Engine " << engine.name << " has weekly update interval and a previous update. Checking if an update is necessary.";

		QDate today = QDate::currentDate();
		QDate lastUpdate = lastUpdateDt.date();

		return !(today.year() == lastUpdate.year() && today.month() == lastUpdate.month() && today.weekNumber() == lastUpdate.weekNumber());
	}
	case UpdateCustom:
	{
		QString s = Movida::settings().value(QString("plugins/blue/engines/%1/updated").arg(engine.name)).toString();
		QDateTime lastUpdate = s.isEmpty() ? QDateTime() : QDateTime::fromString(s, Qt::ISODate);
		if (!lastUpdate.isValid()) {
			iLog() << "MpiBlue:: Engine " << engine.name << " has custom update interval of " << engine.updateIntervalHours << "h but no previous update. Update check required.";
			return true;
		}

		iLog() << "MpiBlue:: Engine " << engine.name << " has custom update interval of " << engine.updateIntervalHours << "h and a previous update. Checking if an update is necessary.";

		QDateTime now = QDateTime::currentDateTime();

		int secDelta = lastUpdate.secsTo(now);
		int secDeltaH = (int) floor((float)secDelta / 3600.0f);

		return secDeltaH >= engine.updateIntervalHours;
	}
	default: ;
	}

	return false;
}

//! Locates the latest version of an engine's scripts and sets the absolute path
void MpiBlue::setScriptPaths(MpiBlue::Engine* engine)
{
	QString path = MpiBlue::locateScriptPath(engine->resultsScript);
	engine->resultsScript = path;
	path = MpiBlue::locateScriptPath(engine->importScript);
	engine->importScript = path;
}

//! Returns the absolute, localized, clean path of the possibly most updated version of a script. (phew!)
QString MpiBlue::locateScriptPath(const QString& name)
{
	Q_ASSERT(MpiBluePlugin::instance);

	// Search order: plugin's user data store, plugin's global data store

	QString filename;

	// plugin's user data store
	QString dataStore = MpiBluePlugin::instance->dataStore(Movida::UserScope);
	filename = QString(dataStore).append(name);
	if (QFile::exists(filename) && MpiBlue::isValidScriptFile(filename) == ValidScript)
		return MvdCore::toLocalFilePath(filename);

	// global data store
	dataStore = MpiBluePlugin::instance->dataStore(Movida::SystemScope);
	filename = QString(dataStore).append(name);
	if (QFile::exists(filename) && MpiBlue::isValidScriptFile(filename) == ValidScript)
		return MvdCore::toLocalFilePath(filename);

	return QString();
}

//! Checks whether \p path points to a (possibly) valid script file by verifying the signature.
MpiBlue::ScriptStatus MpiBlue::isValidScriptFile(const QString& path)
{
	QFile* file = new QFile(path);
	if (!file->open(QIODevice::ReadOnly))
	{
		eLog() << "MpiMovieImport: Failed to open script file: " << path;
		delete file;
		return InvalidScript;
	}
	
	QTextStream stream(file);
	MpiBlue::ScriptStatus res = isValidScriptFile(stream);
	delete file;
	if (res != ValidScript)
		eLog() << "MpiMovieImport: Invalid script file: " << path;
	return res;
}
	
//! Checks whether a temporary file points to a (possibly) valid script file by verifying the signature.
MpiBlue::ScriptStatus MpiBlue::isValidScriptFile(QTemporaryFile* tempFile, bool httpNotModified)
{
	if (httpNotModified)
		return NoUpdatedScript;
	
	QTextStream stream(tempFile);
	MpiBlue::ScriptStatus res = isValidScriptFile(stream);
	if (res != ValidScript)
		eLog() << "MpiMovieImport: Downloaded file is not a valid script file.";
	return res;
}
	
//! \internal
MpiBlue::ScriptStatus MpiBlue::isValidScriptFile(QTextStream& stream)
{
	QString signature = MvdCore::parameter("plugins/blue/script-signature").toString();
	
	QString line;
	bool valid = false;
	int maxLines = 10;
	int lineCount = 0;
	while (++lineCount <= maxLines && !valid && !(line = stream.readLine()).isNull()) {
		valid = line.contains(signature);
	}

	return valid ? ValidScript : InvalidScript;
}

/*!	Returns a temporary directory that the plugin actions can use to store temporary data.
	The directory is created when calling this method for the first time and it uses
	the Movida::paths().generateTempDir() method.
	The directory and its contents are deleted when the plugin is unloaded.
*/
QString MpiBlue::tempDir()
{
	if (mTempDir.isEmpty())
		mTempDir = Movida::paths().generateTempDir();
	return mTempDir;
}
