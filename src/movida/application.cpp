/**************************************************************************
** Filename: application.cpp
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

#include "application.h"
#include "guiglobal.h"
#include "core.h"
#include "logger.h"
#include "settings.h"
#include "pathresolver.h"
#include "mainwindow.h"
#include <QtGlobal>
#include <QTranslator>
#include <QTextCodec>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <iostream>
#include <cstdlib>

#if defined(Q_WS_WIN)
#include <windows.h>
#endif

//! \todo Add option to reset preferences file.

#define MVD_ARG_VERSION "--version"
#define MVD_ARG_HELP "--help"
#define MVD_ARG_LANG "--lang"
#define MVD_ARG_AVAILLANG "--langs-available"
#define MVD_ARG_NOSPLASH "--no-splash"
#define MVD_ARG_NOGUI "--no-gui"
#define MVD_ARG_DISPLAY "--display"

#define MVD_ARG_VERSION_SHORT "-v"
#define MVD_ARG_HELP_SHORT "-h"
#define MVD_ARG_LANG_SHORT "-l"
#define MVD_ARG_AVAILLANG_SHORT "-la"
#define MVD_ARG_NOSPLASH_SHORT "-ns"
#define MVD_ARG_NOGUI_SHORT "-g"
#define MVD_ARG_DISPLAY_SHORT "-d"

// Qt wants -display not --display or -d
#define MVD_ARG_DISPLAY_QT "-display"

// Windows specific options, allows to display a console windows
extern const char MVD_ARG_CONSOLE[] =  "--console";
extern const char MVD_ARG_CONSOLE_SHORT[] = "-cl";

using namespace Movida;
bool MvdApplication::UseGui = false;
MvdApplication* Movida::Application = 0;

MvdApplication::MvdApplication(int& argc, char** argv)
: QApplication(argc, argv)
{
	Movida::Application = this;

	setApplicationName("Movida");
	setOrganizationName("BlueSoft");
	setOrganizationDomain("movida.sourceforge.net");
}

void MvdApplication::initLanguage()
{
	QStringList langs = getLanguage(QString(mLanguage));

	if (!langs.isEmpty())
		installTranslators(langs);
}

void MvdApplication::parseCommandLine()
{
	mShowSplash = true;
	bool usage = false;
	bool header = false;
	bool availLangs = false;
	bool version = false;

	QStringList argv = arguments();
	int argc = argv.size();

	for (int i = 1; i < argc; ++i) {

		const QString& arg = argv[i];

		if ((arg == MVD_ARG_LANG || arg == MVD_ARG_LANG_SHORT) && (++i < argc)) {
			mLanguage = argv[i];
		} else if (arg == MVD_ARG_VERSION || arg == MVD_ARG_VERSION_SHORT) {
			header = true;
			version = true;
		} else if (arg == MVD_ARG_HELP || arg == MVD_ARG_HELP_SHORT) {
			header = true;
			usage = true;
		} else if (arg == MVD_ARG_AVAILLANG || arg == MVD_ARG_AVAILLANG_SHORT) {
			header = true;
			availLangs = true;
		}
	}

	// Init translations
	initLanguage();

	// Show command line help
	if (header)
		showHeader();
	if (version)
		showVersion();
	if (availLangs)
		showAvailableLanguages();
	if (usage)
		showUsage();
	// Don't run the GUI init process called from main.cpp, and return
	if (!header)
		MvdApplication::UseGui = true;
	else
		return;

	// We are going to run something other than command line help
	for (int i = 1; i < argc; i++) {
		QString arg = argv[i];

		if ((arg == MVD_ARG_LANG || arg == MVD_ARG_LANG_SHORT) && (++i < argc)) {
			continue;
		} else if ( arg == MVD_ARG_CONSOLE || arg == MVD_ARG_CONSOLE_SHORT ) {
			continue;
		} else if (arg == MVD_ARG_NOSPLASH || arg == MVD_ARG_NOSPLASH_SHORT) {
			mShowSplash = false;
		} else if (arg == MVD_ARG_NOGUI || arg == MVD_ARG_NOGUI_SHORT) {
			MvdApplication::UseGui = false;
		} else if ((arg == MVD_ARG_DISPLAY || arg == MVD_ARG_DISPLAY_SHORT || arg == MVD_ARG_DISPLAY_QT) && ++i < argc) {
			// allow setting of display, QT expects the option -display <display_name> so we discard the
			// last argument. FIXME: Qt only understands -display not --display and -d , we need to work
			// around this.
		} else if (strncmp(qPrintable(arg), "-psn_", 4) == 0) {
			// Andreas Vox: Qt/Mac has -psn_blah flags that must be accepted.
		} else {
			mFile = QFile::decodeName(argv[i].toLatin1().constData());
			if (!QFileInfo(mFile).exists()) {
				showHeader();
				if (mFile.left(1) == "-" || mFile.left(2) == "--") {
					std::cout << tr("Invalid argument: ").toLatin1().constData() << mFile.toLatin1().constData() << std::endl;
				} else {
					std::cout << tr("File %1 does not exist, aborting.").arg(mFile).toLatin1().constData() << std::endl;
				}
				mFile.clear();
				showUsage();
				MvdApplication::UseGui = false;
				return;
			}
		}
	}
}

int MvdApplication::init()
{
	processEvents();

	if (MvdApplication::UseGui) {
		MvdLogger::setUseHtml(true);

		if (!MvdCore::initCore()) {
			QMessageBox::warning(0, MVD_CAPTION, tr("Failed to initialize the application.\nPlease see the log file for details."));
			exit(MVD_ERROR_INIT);
		}

		mMainWindow = new MvdMainWindow;
		setActiveWindow(mMainWindow);
		connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));

		mMainWindow->show();

		if (!mFile.isEmpty())
			mMainWindow->loadCollection(mFile);
		else mMainWindow->setFocus();
	}

	return MVD_EXIT_SUCCESS;
}

QStringList MvdApplication::getLanguage(QString lang)
{
	QStringList langs;

	// read the locales
	if (!lang.isEmpty())
		langs.append(lang);

	// add in user preferences lang, only overridden by lang command line option
	QString prefLang = settings().value("movida/general/language").toString();
	if (!prefLang.isEmpty())
		langs.append(prefLang);

	if (!(lang = MvdCore::env("LC_ALL")).isEmpty())
		langs.append(lang);
	if (!(lang = MvdCore::env("LC_MESSAGES")).isEmpty())
		langs.append(lang);
	if (!(lang = MvdCore::env("LANG")).isEmpty())
		langs.append(lang);

#if defined(Q_WS_WIN)
	wchar_t out[256];
	QString language, sublanguage;
	LCID lcIdo = GetUserDefaultLCID();
	WORD sortId = SORTIDFROMLCID(lcIdo);
	LANGID langId = GetUserDefaultUILanguage();
	LCID lcIdn = MAKELCID(langId, sortId);
	if ( GetLocaleInfoW(lcIdn, LOCALE_SISO639LANGNAME , out, 255) ) {
		language = QString::fromUtf16( (ushort*)out );
		if ( GetLocaleInfoW(lcIdn, LOCALE_SISO3166CTRYNAME, out, 255) ) {
			sublanguage = QString::fromUtf16( (ushort*)out ).toLower();
			lang = language;
			if ( sublanguage != language && !sublanguage.isEmpty() )
				lang += "_" + sublanguage.toUpper();
			langs.append(lang);
		}
	}
#endif // Q_WS_WIN

	langs.append(QLocale::languageToString(QLocale::system().language()));

	// remove duplicate entries...
	for (QStringList::Iterator it = langs.begin(); it != langs.end(); ++it)
		if (langs.contains(*it))
			it = langs.erase(it);

	return langs;
}

void MvdApplication::installTranslators(const QStringList& langs)
{
	static QTranslator* trans = 0;

	if (trans) {
		removeTranslator(trans);
		delete trans;
	}

	trans = new QTranslator(0);
	bool loaded = false;
	QString lang;

	// Look for user specific translations first
	QString path = paths().resourcesDir(Movida::UserScope).append("Translations");
	if (QFile::exists(path)) {
		path.append("movida");
		for (int i = 0; i < langs.size() && !loaded; ++i) {
			QString lang = langs.at(i).left(5);
			if (lang == "en")
				break;
			else loaded = trans->load(QString(path + '.' + lang), ".");
		}
	}

	// Try with system wide translations (if any)
	if (!loaded) {
		path = paths().resourcesDir(Movida::SystemScope).append("Translations");
		if (QFile::exists(path)) {
			path.append("movida");
			QString lang;
			for (int i = 0; i < langs.size() && !loaded; ++i) {
				QString lang = langs.at(i).left(5);
				if (lang == "en")
					break;
				else loaded = trans->load(QString(path + '.' + lang), ".");
			}
		}
	}

	if (loaded) {
		installTranslator(trans);
		mGuiLanguage = lang;
	}
}

void MvdApplication::changeGuiLanguage(const QString& newGUILang)
{
	QStringList newLangs;
	if (newGUILang.isEmpty())
		newLangs.append("en");
	else
		newLangs.append(newGUILang);
	installTranslators(newLangs);
}

static void printArgLine(QTextStream& ts, const char* smallArg, const char* fullArg, const QString& desc)
{
	const char* lineformat = "  %1, %2 %3";
	const int saw = 3;   // Short argument width
	const int aw = -18;  // Argument width (negative is left aligned)
	QString line = QString(lineformat)
		.arg(smallArg, saw)
		.arg(fullArg, aw)
		.arg(desc);
	ts << line;
	endl(ts);
}

void MvdApplication::showUsage()
{
	QFile f;
	f.open(stderr, QIODevice::WriteOnly);
	QTextStream ts(&f);
	ts << tr("Usage: movida [option ... ] [file]") ; endl(ts);
	ts << tr("Options:") ; endl(ts);
	printArgLine(ts, MVD_ARG_HELP_SHORT, MVD_ARG_HELP,
		tr("Print help (this message) and exit") );
	printArgLine(ts, MVD_ARG_LANG_SHORT, MVD_ARG_LANG,
		tr("Uses xx as shortcut for a language, eg `en' or `de'") );
	printArgLine(ts, MVD_ARG_AVAILLANG_SHORT, MVD_ARG_AVAILLANG,
		tr("List the currently installed interface languages") );
	printArgLine(ts, MVD_ARG_NOSPLASH_SHORT, MVD_ARG_NOSPLASH,
		tr("Do not show the splashscreen on startup") );
	printArgLine(ts, MVD_ARG_VERSION_SHORT, MVD_ARG_VERSION,
		tr("Output version information and exit") );
#if defined(Q_WS_WIN) && !defined(_CONSOLE)
	printArgLine(ts, MVD_ARG_CONSOLE_SHORT, MVD_ARG_CONSOLE,
		tr("Display a console window") );
#endif

	endl(ts);
}

void MvdApplication::showAvailableLanguages()
{
	QFile f;
	f.open(stderr, QIODevice::WriteOnly);
	QTextStream ts(&f);
	ts << tr("Installed interface languages for movida are as follows:"); endl(ts);
	endl(ts);

	//! \todo Print installed languages!
	/*LanguageManager langMgr;
	langMgr.init();
	langMgr.printInstalledList();*/

	endl(ts);
	ts << tr("To override the default language choice:"); endl(ts);
	ts << tr("movida -l xx or movida --lang xx, where xx is the language of choice."); endl(ts);
}

void MvdApplication::showVersion()
{
	std::cout << tr("movida version").toLatin1().constData() << " " << MVD_VER_MAJOR << "." << MVD_VER_MINOR << std::endl;
}

void MvdApplication::showHeader()
{
	QFile f;
	f.open(stderr, QIODevice::WriteOnly);
	QTextStream ts(&f);
	endl(ts);
	QString heading( tr("movida, the free movie collection manager") );
	// Build a separator of ----s the same width as the heading
	QString separator = QString("").rightJustified(heading.length(),'-');
	// Then output the heading, separator, and docs/www/etc info in an aligned table
	const int urlwidth = 23;
	const int descwidth = -(heading.length() - urlwidth - 1);
	ts << heading; endl(ts);
	ts << separator; endl(ts);
	ts << QString("%1 %2").arg( tr("Homepage")+":",      descwidth).arg("http://movida.sourceforge.net" ); endl(ts);
	ts << QString("%1 %2").arg( tr("Documentation")+":", descwidth).arg("http://movida.sourceforge.net/help"); endl(ts);
	ts << QString("%1 %2").arg( tr("Download")+":",          descwidth).arg("http://movida.sourceforge.net/download"); endl(ts);
	endl(ts);
}
