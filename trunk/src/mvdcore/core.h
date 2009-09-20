/**************************************************************************
** Filename: core.h
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

#ifndef MVD_CORE_H
#define MVD_CORE_H

#include "global.h"

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QtGlobal>
#ifdef Q_OS_WIN32
# include <qt_windows.h>
#endif
#include <libxml/xmlerror.h>

class MvdMovieCollection;
class MvdPluginContext;
class MvdPluginInterface;

namespace Movida {
enum MessageType {
    InformationMessage,
    WarningMessage,
    ErrorMessage
};

MVD_EXPORT extern void info(const QString &msg);
MVD_EXPORT extern void warning(const QString &msg);
MVD_EXPORT extern void error(const QString &msg);

typedef void (*MessageHandler)(MessageType, const QString &msg);
MVD_EXPORT extern void registerMessageHandler(Movida::MessageHandler handler);
} // Movida namespace

class MVD_EXPORT MvdActionUrl
{
public:
    inline bool isValid() const { return !action.isEmpty(); }

    operator QString() const;

    QString action;
    QString parameter;
};


class MVD_EXPORT MvdCore : public QObject
{
    Q_OBJECT

public:
    enum LocateOption {
        NoLocateOption = 0x00, IncludeApplicationPath = 0x01
#ifdef Q_WS_WIN32
        , IncludeRegistryCache = 0x02
#endif
    };
    Q_DECLARE_FLAGS(LocateOptions, LocateOption)

    static MvdCore &instance();

    bool isInitialized() const;
    void loadStatus();
    void storeStatus();

    MvdMovieCollection *createNewCollection();
    MvdMovieCollection *currentCollection() const;

    QVariant parameter(const QString &name);
    void registerParameters(const QHash<QString, QVariant> &p);

    int pluginCount() const;
    MvdPluginInterface *plugin(int index) const;
    MvdPluginInterface *findPlugin(const QString &id) const;
    QList<MvdPluginInterface *> plugins() const;

signals:
    void pluginsLoaded();
    void pluginsUnloaded();
    void collectionCreated(MvdMovieCollection* c);

public:
    //////////////////////////////////////////
    // Static utility methods
    //////////////////////////////////////////

    static mvdid atoid(const char *c, bool *ok = 0);

    static QString replaceNewLine(QString text);

    static QByteArray toLatin1PercentEncoding(const QString &input,
        const QByteArray &exclude = QByteArray(),
        const QByteArray &include = QByteArray());

    static QString decodeXmlEntities(const char *s);
    static QString decodeXmlEntities(QString s);

    static MvdActionUrl parseActionUrl(const QString &url);
    static MvdActionUrl parseActionUrl(const QUrl &url) { return parseActionUrl(url.toString()); }

    static QString locateApplication(QString name, LocateOptions options = IncludeApplicationPath);

    static QString env(const QString &s, Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    static void replaceEnvVariables(QString &s);

    static QString& sanitizePath(QString &path);
    static QString sanitizedPath(const QString &path);

    static QString toLocalFilePath(QString s, bool considerDirectory = false);
    static QString toQtFilePath(QString s, bool considerDirectory = false);
    static QString fixedFilePath(QString s);

    static bool isValidYear(QString s);

    static MvdPluginContext *pluginContext();

#ifdef Q_OS_WIN32
    static QString MvdCore::getWindowsRegString(HKEY key, const QString &subKey);
#endif

private:
    MvdCore();
    MvdCore(const MvdCore &);
    MvdCore &operator=(const MvdCore &);
    virtual ~MvdCore();

    void initialize();
    static void create();
    static volatile MvdCore *mInstance;
    static bool mDestroyed;
    static qint8 mCoreInitOk;

    friend class Private;
    class Private;
    Private *d;

    static MvdPluginContext *PluginContext;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdCore::LocateOptions)

namespace Movida {
void xmlStructuredErrorHandler(void *userData, xmlErrorPtr error);
extern MVD_EXPORT MvdCore &core();
}

#endif // MVD_CORE_H
