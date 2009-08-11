/**************************************************************************
** Filename: core.cpp
**
** Copyright (C) 2007-2009 Angius Fabrizio. All rights reserved.
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

#include "core.h"

#include "logger.h"
#include "pathresolver.h"
#include "plugininterface.h"
#include "settings.h"
#include "shareddata.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDate>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLocale>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QVarLengthArray>
#include <QtCore/QtGlobal>

#include <cstdlib>
#include <iostream>


using namespace Movida;

/*!
    \class MvdCore core.h
    \ingroup MvdCore

    \brief Application core: initialization and utility methods.
*/

namespace {

Movida::MessageHandler msgHandler = 0;
Movida::MessageHandler oldMsgHandler = 0;

void dispatchMessage(Movida::MessageType t, const QString &m)
{
    if (!msgHandler)
        return;
    msgHandler(t, m);
}

void defaultMessageHandler(Movida::MessageType t, const QString &m)
{
    switch (t) {
        case Movida::InformationMessage:
            std::cout << "[INFO]: " << qPrintable(m) << std::endl; break;

        case Movida::WarningMessage:
            std::cout << "[WARNING]: " << qPrintable(m) << std::endl; break;

        case Movida::ErrorMessage:
            std::cout << "[ERROR]: " << qPrintable(m) << std::endl; break;

        default:
            ;
    }
}

} // anonymous namespace


/************************************************************************
    MvdCore::Private
 *************************************************************************/

//! \internal
class MvdCore::Private
{
public:
    //! \internal
    Private()
    {
        parameters.insert("mvdcore/max-rating", 5);
        parameters.insert("mvdcore/max-running-time", 999);
        parameters.insert("mvdcore/imdb-id-length", 7);
        parameters.insert("mvdcore/imdb-id-regexp", "[0-9]{7}");
        parameters.insert("mvdcore/imdb-id-mask", "9999999;0");
        parameters.insert("mvdcore/imdb-movie-url", "http://akas.imdb.com/title/tt%1");
        parameters.insert("mvdcore/extra-attributes/import-date", "import-date");

        // The Oberammergau Passion Play of 1898 was the first commercial
        // motion picture ever produced. :)
        // It's quite hard that someone will add it to Movida, but its a
        // nice date ;)
        parameters.insert("mvdcore/min-movie-year", 1898);

        // Movie posters bigger than this will be scaled.
        parameters.insert("mvdcore/max-poster-kb", 512);
        parameters.insert("mvdcore/max-poster-size", QSize(400, 150));

        parameters.insert("mvdcore/website-url", "http://movida.42cows.org");

        // Max length for some string values
        // This should be enough even for "Night of the Day of the Dawn of the Son of the Bride of the Return of the Revenge of the Terror of the Attack of the Evil, Mutant, Alien, Flesh Eating, Hellbound, Zombified Living Dead Part 2: In Shocking 2-D" :D
        parameters.insert("mvdcore/max-edit-length", 256);

        // 2:02
        parameters.insert("mvdcore/running-time-format", "h'h' mm'min'");

        // Register default message handler
        Movida::registerMessageHandler(defaultMessageHandler);
    }

    QHash<QString, QVariant> parameters;
};

//! \internal
MvdCore::Private *MvdCore::d = 0;

/************************************************************************
    MvdCore
 *************************************************************************/

//! \internal
bool MvdCore::MvdCoreInitOk = false;
MvdPluginContext *MvdCore::PluginContext = 0;

/*!
    Possibly call this from the main() routine. It will attempt to initialize
    any required directories, files and libraries.
    Exit if the method returns an error. Running Movida if some error occurs
    may lead to an unpredictable behavior.
    Subsequent calls to this method have no effect, they simply return the
    exit status of the first initialization.
*/
bool MvdCore::initCore()
{
    if (d != 0)
        return MvdCoreInitOk;

    // Pre-initialize MvdPathResolver to create missing application directories
    if (!paths().isInitialized())
        return false;

    QString appPaths;
    appPaths.append(MVD_LINEBREAK).append("  Log file: ").append(paths().logFile());
    appPaths.append(MVD_LINEBREAK).append("  Settings file: ").append(paths().settingsFile());
    appPaths.append(MVD_LINEBREAK).append("  User resources directory: ").append(paths().resourcesDir(Movida::UserScope));
    appPaths.append(MVD_LINEBREAK).append("  System resources directory: ").append(paths().resourcesDir(Movida::SystemScope));
    appPaths.append(MVD_LINEBREAK).append("  Temporary directory: ").append(paths().tempDir());
    iLog() << "MvdCore: Application paths:" << appPaths;

    d = new Private;

    //! \todo check library versions?

    // Init libxml2
    xmlSetStructuredErrorFunc(NULL, Movida::xmlStructuredErrorHandler);

    MvdCoreInitOk = true;
    return true;
}

/*!
    Call this once on application startup to load application preferences,
    and more.
    Subsequent calls to this method have no effect.
*/
void MvdCore::loadStatus()
{
    // Load application preferences. Set defaults and overwrite with user settings.
    Q_UNUSED(settings());
}

/*!
    Call this once on application termination to store application preferences
    and more.
    Subsequent calls to this method have no effect.
*/
void MvdCore::storeStatus()
{
    Q_ASSERT(d);
    delete d;
    d = 0;
    Movida::paths().removeDirectoryTree(Movida::paths().tempDir());
}

/*!
    Returns the value of a specific application parameter.
    Returns a null QVariant and prints an error message using
    qDebug (this is for developers only, so it makes no sense to
    use the log here - no unbound variables should be used in public
    releases) if the parameter name is not bound.

    Values should be used as constants and client libraries
    should register their parameters at startup.
    This works slower than using #defines or static constants,
    but it speeds up compilation, adds binary compatibility and
    a centralized control point for application wide constants.
*/
QVariant MvdCore::parameter(const QString &name)
{
    Q_ASSERT(d);

    QHash<QString, QVariant>::ConstIterator it = d->parameters.find(name);
    if (it != d->parameters.constEnd())
        return it.value();

    qDebug() << "Request for unbound parameter" << name;
    return QVariant();
}

/*!
    Registers application parameters.
    Please use a library unique prefix for the names.
    movida uses the following format, so consider using the same pattern for consistency:
    "LIBRARY/SOME-PARAMETER-NAME" (i.e. "mvdcore/imdb-id-regexp" or "movida/imdb-id-regexp")
*/
void MvdCore::registerParameters(const QHash<QString, QVariant> &p)
{
    Q_ASSERT(d);
    d->parameters.unite(p);
}

/*!
    Similar to atoi() for unsigned 32bit values.
*/
mvdid MvdCore::atoid(const char *c, bool *ok)
{
    QString s(c);
    bool myOk;
    mvdid i;

    i = s.toUInt(&myOk);
    if (ok) *ok = myOk;
    return myOk ? i : MvdNull;
}

/*!
    Replaces occurrences of \n with QChar::LineSeparator.
*/
QString MvdCore::replaceNewLine(QString text)
{
    const QChar nl = QLatin1Char('\n');

    for (int i = 0; i < text.count(); ++i)
        if (text.at(i) == nl)
            text[i] = QChar::LineSeparator;
    return text;
}

//! \internal Some code from Qt 4.2.0 qurl.cpp required by toLatin1PercentEncoding()
namespace {
inline bool q_strchr(const char str[], char chr)
{
    if (!str) return false;

    const char *ptr = str;
    char c;
    while ((c = *ptr++))
        if (c == chr)
            return true;
    return false;
}

static const char hexnumbers[] = "0123456789ABCDEF";
static inline char toHex(char c)
{
    return hexnumbers[c & 0xf];
}

}

/*!
    Same as QUrl::toPercentEncoding except that it does not convert the
    string to UTF8 but to Latin1 (as required by some servers).

    From the QUrl documentation:

    Returns an encoded copy of input. input is first converted to UTF-8,
    and all ASCII-characters that are not in the unreserved group are percent
    encoded. To prevent characters from being percent encoded pass them to
    exclude. To force characters to be percent encoded pass them to include.

    Unreserved is defined as: ALPHA / DIGIT / "-" / "." / "_" / "~"
*/
QByteArray MvdCore::toLatin1PercentEncoding(const QString &input,
    const QByteArray &exclude, const QByteArray &include)
{
    // The code is the same as QUrl::toPercentEncoding as to Qt 4.2.0.
    // The following line is actually the only difference (except for
    // the removed debug prints) with the original implementation.
    QByteArray tmp = input.toLatin1();

    QVarLengthArray<char> output(tmp.size() * 3);

    int len = tmp.count();
    char *data = output.data();
    const char *inputData = tmp.constData();
    int length = 0;

    const char *dontEncode = 0;
    if (!exclude.isEmpty()) dontEncode = exclude.constData();


    if (include.isEmpty()) {
        for (int i = 0; i < len; ++i) {
            unsigned char c = *inputData++;
            if ((c >= 0x61 && c <= 0x7A)     // ALPHA
                || (c >= 0x41 && c <= 0x5A)     // ALPHA
                || (c >= 0x30 && c <= 0x39)     // DIGIT
                || c == 0x2D     // -
                || c == 0x2E     // .
                || c == 0x5F     // _
                || c == 0x7E     // ~
                || q_strchr(dontEncode, c)) {
                data[length++] = c;
            } else {
                data[length++] = '%';
                data[length++] = toHex((c & 0xf0) >> 4);
                data[length++] = toHex(c & 0xf);
            }
        }
    } else {
        const char *alsoEncode = include.constData();
        for (int i = 0; i < len; ++i) {
            unsigned char c = *inputData++;
            if (((c >= 0x61 && c <= 0x7A)     // ALPHA
                 || (c >= 0x41 && c <= 0x5A)   // ALPHA
                 || (c >= 0x30 && c <= 0x39)   // DIGIT
                 || c == 0x2D   // -
                 || c == 0x2E   // .
                 || c == 0x5F   // _
                 || c == 0x7E   // ~
                 || q_strchr(dontEncode, c))
                && !q_strchr(alsoEncode, c)) {
                data[length++] = c;
            } else {
                data[length++] = '%';
                data[length++] = toHex((c & 0xf0) >> 4);
                data[length++] = toHex(c & 0xf);
            }
        }
    }

    return QByteArray(output.data(), length);
}

/*!
    Replaces entities in XML/HTML text.
    Convenience method, expects the const char* to contain a UTF-8 string.
*/
QString MvdCore::decodeXmlEntities(const char *s)
{
    return s ? decodeXmlEntities(QString::fromUtf8(s)) : QString();
}

/*!
    Replaces entities in XML/HTML text.
*/
QString MvdCore::decodeXmlEntities(QString s)
{
    if (s.isEmpty())
        return s;

    QRegExp rx("&#(x)?([0-9a-f]+);");
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    int pos = rx.indexIn(s);
    while (pos >= 0) {
        QString num;
        bool isHex = false;
        if (rx.numCaptures() == 2) {
            num = rx.cap(2);
            isHex = true;
        } else num = rx.cap(1);
        s.replace(pos, rx.matchedLength(), QChar((num.toInt(0, isHex ? 16 : 10))));
        pos = rx.indexIn(s, pos + 1);
    }

    s.replace("&nbsp;", QChar(' '));
    return s;
}

//! Redirects libxml2 structured errors to the application log.
void Movida::xmlStructuredErrorHandler(void *userData, xmlErrorPtr error)
{
    Q_UNUSED(userData);

    QString msg = QString("(libxml2@%1:%2) ").arg(error->line).arg(error->int2);
    msg.append(QString(error->message).trimmed());

    switch (error->level) {
        case XML_ERR_NONE:
            iLog() << msg; break;

        case XML_ERR_WARNING:
            wLog() << msg; break;

        default:
            eLog() << msg;
    }
}

MvdActionUrl MvdCore::parseActionUrl(const QString &url)
{
    MvdActionUrl a;

    QRegExp rx("^movida://([^/]+)(?:/(.+))?$");

    rx.setMinimal(true);
    if (rx.indexIn(url) >= 0) {
        a.action = rx.cap(1);
        a.parameter = rx.cap(2);
    }

    return a;
}

/*!
    Attempts to locate an application with the given name.
    \p name must not include platform dependent suffixes (i.e. ".exe").

    The application is first searched in the movida application directory
    (unless \p searchInAppDirPath is true) and then
    in the directories included in the PATH environment variable.

    Returns the path to the application (filename included) or an empty QString on failure.
*/
QString MvdCore::locateApplication(QString name, LocateOptions options)
{
    name = name.trimmed();
    if (name.isEmpty())
        return QString();

    bool searchInAppDirPath = (options & IncludeApplicationPath);

#ifdef Q_OS_WIN
    name.append(".exe");
#endif

    //! \todo On Mac OS X this will point to the directory actually containing the executable, which may be inside of an application bundle (if the application is bundled).
    QString appPath = searchInAppDirPath ? QCoreApplication::applicationDirPath() : QString();
    if (!appPath.isEmpty()) {
        QFileInfo fi(appPath.append("/").append(name));
        if (fi.exists() && fi.isExecutable())
            return fi.absoluteFilePath();
    }

    QString pathEnv = env("path", Qt::CaseInsensitive);

#ifdef Q_OS_WIN
    QChar pathSep(';');
#else
    QChar pathSep(':');
#endif

    QStringList pathList = pathEnv.split(pathSep, QString::SkipEmptyParts);
    for (int i = 0; i < pathList.size(); ++i) {
        QString p = pathList.at(i);
        QFileInfo fi(p.append("/").append(name));
        if (fi.exists() && fi.isExecutable())
            return fi.absoluteFilePath();
    }

#ifdef Q_OS_WIN32 // Get a strong coffee and now on with some Win32 API nightmare
    if (options & IncludeRegistryCache) {
        QString path = QString("Software\\Classes\\Applications\\%1\\shell\\open\\command").arg(name);
        HKEY k;
        int r;
        QT_WA({
                r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (TCHAR *)path.utf16(), 0, KEY_READ, &k);
            }, {
                r = RegOpenKeyExA(HKEY_LOCAL_MACHINE, (LPCSTR)path.toLatin1().constData(), 0, KEY_READ, &k);
            });
        path.clear();

        if (r == ERROR_SUCCESS) {
            path = MvdCore::getWindowsRegString(k, QLatin1String(""));
            if (!path.isEmpty()) {
                // Paths are often in the following form: "c:\overly\long\path\to\my\app\app.exe" "%1"
                // ...so we need to extract the actual path :-(
                QRegExp rx(QString("\"(.*%1)\"").arg(name));
                rx.setMinimal(true);
                if (rx.indexIn(path) != -1)
                    path = rx.cap(1);
            }
        }
        RegCloseKey(k);
        if (!path.isEmpty() && QFile::exists(path)) {
            return path;
        }
    }
#endif

    return QString();
}

//! Returns the value of an environment variable.
QString MvdCore::env(const QString &s, Qt::CaseSensitivity cs)
{
    QRegExp rx(QString("^%1=(.*)$").arg(s));

    rx.setCaseSensitivity(cs);

    QStringList envList = QProcess::systemEnvironment();
    for (int i = 0; i < envList.size(); ++i) {
        QString e = envList.at(i);
        if (rx.exactMatch(e))
            return rx.cap(1);
    }

    return QString();
}

//! Cleans a file path and replaces path separators with the separators used on the current platform.
QString MvdCore::toLocalFilePath(QString s, bool considerDirectory)
{
    s = QDir::toNativeSeparators(QDir::cleanPath(s));
    if (considerDirectory && !s.endsWith(QDir::separator()))
        s.append(QDir::separator());
    return s;
}

/*! Replaces environment variables in the form $NAME on unix based systems
    or %NAME% with the contents of that variable.
*/
void MvdCore::replaceEnvVariables(QString &s)
{
#ifdef Q_OS_WIN32
    QRegExp rx("%(\\w*)%");
#else
    QRegExp rx("$(\\w*)");
#endif
    QString out;
    int pos = rx.indexIn(s);
    while (pos >= 0) {
        QString var = rx.cap(1);
        QString val = QString::fromLatin1(::getenv(var.toLatin1().constData()));
        s.replace(pos, rx.matchedLength(), val);
        pos = rx.indexIn(s, pos + 1);
    }
}

//! Replaces special strings like ~ or environment variables from a file path.
QString MvdCore::fixedFilePath(QString s)
{
#ifndef Q_OS_WIN32
    s.replace(QLatin1String("~"), QDir::homePath());
#endif
    replaceEnvVariables(s);
    return s;
}

//! Returns true if the string represents a valid movie year.
bool MvdCore::isValidYear(QString s)
{
    bool ok;
    int n = s.toInt(&ok);

    if (!ok)
        return false;

    int minY = MvdCore::parameter("mvdcore/min-movie-year").toInt();
    int maxY = QDate::currentDate().year();

    return !(n< minY || n > maxY);
}

/*! Returns the application wide unique plugin context. The application using movida core
    is supposed to initialize and keep the context up-to-date (e.g. set the collection pointer
    to the currently open collection or to a new collection) before calling any plugin action.

    \warning Remember that plugins are allowed to assert that the context is valid and contains
    valid (non null) pointers only!
*/
MvdPluginContext *MvdCore::pluginContext()
{
    if (!MvdCore::PluginContext)
        MvdCore::PluginContext = new MvdPluginContext;
    return MvdCore::PluginContext;
}

#ifdef Q_OS_WIN32

//! \internal Borrowed from Qt 4.3.2 -- Copyright (C) 1992-2007 Trolltech ASA. All rights reserved.
QString MvdCore::getWindowsRegString(HKEY key, const QString &subKey)
{
    QString s;

    QT_WA({
            char buf[1024];
            DWORD bsz = sizeof(buf);
            int r = RegQueryValueEx(key, (TCHAR *)subKey.utf16(), 0, 0, (LPBYTE)buf, &bsz);
            if (r == ERROR_SUCCESS) {
                s = QString::fromUtf16((unsigned short *)buf);
            } else if (r == ERROR_MORE_DATA) {
                char *ptr = new char[bsz + 1];
                r = RegQueryValueEx(key, (TCHAR *)subKey.utf16(), 0, 0, (LPBYTE)ptr, &bsz);
                if (r == ERROR_SUCCESS)
                    s = QLatin1String(ptr);
                delete[] ptr;
            }
        }, {
            char buf[512];
            DWORD bsz = sizeof(buf);
            int r = RegQueryValueExA(key, (LPCSTR)subKey.toLatin1().constData(), 0, 0, (LPBYTE)buf, &bsz);
            if (r == ERROR_SUCCESS) {
                s = QLatin1String(buf);
            } else if (r == ERROR_MORE_DATA) {
                char *ptr = new char[bsz + 1];
                r = RegQueryValueExA(key, (LPCSTR)subKey.toLatin1().constData(), 0, 0, (LPBYTE)ptr, &bsz);
                if (r == ERROR_SUCCESS)
                    s = QLatin1String(ptr);
                delete[] ptr;
            }
        });
    return s;
}

#endif

void Movida::info(const QString &msg)
{
        ::dispatchMessage(InformationMessage, msg);
}

void Movida::warning(const QString &msg)
{
        ::dispatchMessage(WarningMessage, msg);
}

void Movida::error(const QString &msg)
{
        ::dispatchMessage(ErrorMessage, msg);
}

void Movida::registerMessageHandler(MessageHandler handler)
{
    if (!handler) {
        if (::oldMsgHandler) {
                ::msgHandler = ::oldMsgHandler;
                ::oldMsgHandler = 0;
        }
        return;
    }

        ::oldMsgHandler = ::msgHandler;
        ::msgHandler = handler;
}

MvdActionUrl::operator QString() const
{
    if (action.isEmpty()) return QString();
    if (parameter.isEmpty()) return QString();
    QString s = action;
    return s.append(QLatin1Char('/')).append(parameter);
}
