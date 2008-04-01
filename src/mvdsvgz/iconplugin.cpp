/**************************************************************************
** Filename: iconplugin.cpp
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

#include "iconplugin.h"
#include "iconengine.h"

MvdSvgzIconEnginePlugin::MvdSvgzIconEnginePlugin(QObject* parent)
: QIconEnginePluginV2(parent)
{

}

MvdSvgzIconEnginePlugin::~MvdSvgzIconEnginePlugin()
{
}

QIconEngineV2* MvdSvgzIconEnginePlugin::create(const QString& filename)
{
	MvdSvgzIconEngine* engine = new MvdSvgzIconEngine;
	if (!filename.isNull())
		engine->addFile(filename, QSize(), QIcon::Normal, QIcon::On);
	return engine;
}

QStringList MvdSvgzIconEnginePlugin::keys() const
{
	return QStringList() << QLatin1String("svgz");
}

Q_EXPORT_PLUGIN2(MvdSvgzIconEnginePlugin, MvdSvgzIconEnginePlugin)
