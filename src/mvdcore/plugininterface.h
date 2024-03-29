/**************************************************************************
** Filename: plugininterface.h
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

#ifndef MVD_PLUGININTERFACE_H
#define MVD_PLUGININTERFACE_H

#include "global.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtGui/QKeySequence>

class MvdMovieCollection;

class MvdPluginContext
{
public:
    MvdPluginContext()
    { }

    QHash<QString, QVariant> properties;
    QList<mvdid> selectedMovies;
};

class MVD_EXPORT MvdPluginInterface :
    public QObject
{
Q_OBJECT

public:
    enum ActionType {
        GenericAction = 0,
        ImportAction,
        ExportAction
    };
    Q_DECLARE_FLAGS(ActionTypes, ActionType);

    struct PluginInfo {
        QString uniqueId;

        QString name;
        QString author;
        QString websiteUrl;
        QString updateUrl;
        QString version;
        QString description;
    };

    struct PluginAction {
        PluginAction(ActionType t = GenericAction) :
            type(t) { }

        QIcon icon;
        QString text;
        QString helpText;
        QString name;
        ActionTypes type;
        QList<QKeySequence> shortcuts;
    };

    MvdPluginInterface(QObject * parent = 0);
    virtual ~MvdPluginInterface();

    virtual bool init() = 0;
    virtual void unload() = 0;

    virtual QString lastError() const = 0;
    virtual PluginInfo info() const = 0;
    virtual QList<PluginAction> actions() const = 0;

    virtual void actionTriggeredImplementation(const QString &name, const QStringList &parameters) = 0;

    QString dataStore(Movida::Scope scope = Movida::UserScope) const;
    void setDataStore(const QString &path, Movida::Scope scope = Movida::UserScope);

    QString id() const;
    QString name() const;

public slots:
    void actionTriggered(const QString &name, const QStringList &parameters = QStringList());

private:
    class Private;
    Private *d;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(MvdPluginInterface::ActionTypes)

#endif // MVD_PLUGININTERFACE_H
