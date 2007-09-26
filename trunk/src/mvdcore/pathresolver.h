/**************************************************************************
** Filename: pathresolver.h
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

#ifndef MVD_PATHRESOLVER_H
#define MVD_PATHRESOLVER_H

#include "global.h"
#include <QtGlobal>
#include <QString>

class MvdPathResolver_P;

class MVD_EXPORT MvdPathResolver
{
public:
	static MvdPathResolver& instance();
	virtual ~MvdPathResolver();

	bool isInitialized() const;

	QString settingsDir() const;
	QString settingsFile() const;
	QString logFile() const;
	QString tempDir() const;
	QString resourcesDir(Movida::Scope scope = Movida::UserScope) const;
	QString generateTempDir() const;

	static QString applicationDirPath();
	static bool removeDirectoryTree(const QString& dir, const QString& excludeDir = QString());

private:
	MvdPathResolver();
	static MvdPathResolver* mInstance;
	MvdPathResolver_P* d;
};

namespace Movida
{
	MVD_EXPORT extern MvdPathResolver& paths();
	MVD_EXPORT extern int pid();
}

#endif // MVD_PATHRESOLVER_H
