/**************************************************************************
** Filename: blue.cpp
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

#include "blue.h"
#include "core.h"
#include "logger.h"
#include "movieimport.h"
#include "settings.h"
#include "pathresolver.h"
#include <QFile>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

using namespace Movida;

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
	QHash<QString,QVariant> parameters;
	parameters.insert("mvdp://blue.mpi/script-signature", "## movida blue.mpi plugin script ##");
	MvdCore::registerParameters(parameters);
}

MpiBlue::~MpiBlue()
{
}

bool MpiBlue::init()
{
	loadEngines();
	return true;
}

void MpiBlue::unload()
{
}

QString MpiBlue::lastError() const
{
	return QString();
}

MvdPluginInterface::PluginInfo MpiBlue::info() const
{
	MvdPluginInterface::PluginInfo info;
	info.name = tr("Blue plugin");
	info.description = tr("Basic import/export plugin");
	info.author = "Fabrizio Angius";
	info.version = "0.2";
	return info;
}

QList<MvdPluginInterface::PluginAction> MpiBlue::actions() const
{
	QList<MvdPluginInterface::PluginAction> list;
	
	MvdPluginInterface::PluginAction a;
	a.text = tr("IMDb movie import");
	a.helpText = tr("Import movies from the IMDb website.");
	a.name = "imdb-import";
	a.type = MvdPluginInterface::MovieImportAction;
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
	//! \todo Problem: we need to check for updated scripts *and* set the absolute file path (script might be in user or in global directory). AND we would like to do a LAZY update! Problem is that we need to remember what script has been fetched in the plugin (i.e. HERE) and not in the MpiMovieImport class, which is created only when necessary.
	if (loadBundled && !settings().value("plugins/blue/disableBundledEngines").toBool())
		loadEnginesFromFile(":/xml/engines.xml");

	QString externalEngines = dataStore().append("/engines.xml");
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
				xmlChar* attr = xmlGetProp(engineNode, (const xmlChar*)"updateUrl");
				if (attr)
				{
					engine.resultsUrl = QString::fromLatin1((const char*)attr);
					xmlFree(attr);
				}
			}
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "import-script"))
			{
				engine.importScript = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();
				xmlChar* attr = xmlGetProp(engineNode, (const xmlChar*)"updateUrl");
				if (attr)
				{
					engine.importUrl = QString::fromLatin1((const char*)attr);
					xmlFree(attr);
				}
			}
			else if (!xmlStrcmp(engineNode->name, (const xmlChar*) "search-url"))
				engine.searchUrl = QString((const char*)xmlNodeListGetString(doc, engineNode->xmlChildrenNode, 1)).trimmed();

			engineNode = engineNode->next;
		}

		if (engine.name.isEmpty())
			engine.name = tr("Unnamed engine");

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
