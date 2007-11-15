/**************************************************************************
** Filename: application.h
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

#ifndef MVD_APPLICATION_H
#define MVD_APPLICATION_H

#include <QApplication>

class MvdMainWindow;

class MvdApplication : public QApplication
{
	Q_OBJECT

public:
	MvdApplication(int& argc, char** argv);

	int init();
	void initLanguage();
	void parseCommandLine();
	QStringList getLanguage(QString lang);
	void installTranslators(const QStringList & langs);
	void changeGuiLanguage(const QString & lang);
	const QString& currentGuiLanguage() { return mGuiLanguage; };
	bool usingGui() const { return MvdApplication::UseGui; }

	static bool UseGui;

private:
	void showHeader();
	void showVersion();
	void showUsage();
	void showAvailableLanguages();

	MvdMainWindow* mMainWindow;
	QString mLanguage;
	QString mGuiLanguage;
	bool mShowSplash;
	QString mFile;
};

namespace Movida {
	extern MvdApplication* Application;
}

#endif // MVD_APPLICATION_H
