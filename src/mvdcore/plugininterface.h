/**************************************************************************
** Filename: plugininterface.h
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

#ifndef MVD_PLUGININTERFACE_H
#define MVD_PLUGININTERFACE_H

#include "global.h"
#include <QString>
#include <QIcon>
#include <QList>

class MvdPluginInterface_P;
class MvdMovieCollection;

class MvdPluginContext
{
public:
	MvdPluginContext() : collection(0) {}

	MvdMovieCollection* collection;
};

class MVD_EXPORT MvdPluginInterface : public QObject
{
	Q_OBJECT

public:
	enum ActionType
	{
		GenericAction = 0,
		MovieImportAction,
		CollectionImportAction,
		MovieExportAction,
		CollectionExportAction
	};

	struct PluginInfo
	{
		QString name;
		QString author;
		QString websiteUrl;
		QString updateUrl;
		QString version;
		QString description;
	};

	struct PluginAction
	{
		PluginAction() : type(GenericAction) {}

		QIcon icon;
		QString text;
		QString helpText;
		QString name;
		ActionType type;
	};

	MvdPluginInterface(QObject* parent = 0);
	virtual ~MvdPluginInterface();

	virtual bool init() = 0;
	virtual void unload() = 0;

	virtual QString lastError() const = 0;
	virtual PluginInfo info() const = 0;
	virtual QList<PluginAction> actions() const = 0;

	virtual void actionTriggeredImplementation(const QString& name) = 0;

	QString dataStore(Movida::Scope scope = Movida::UserScope) const;
	void setDataStore(const QString& path, Movida::Scope scope = Movida::UserScope);

public slots:
	void actionTriggered(const QString& name);

private:
	MvdPluginInterface_P* d;
};

#endif // MVD_PLUGININTERFACE_H
