/**************************************************************************
** Filename: settings.cpp
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

#include "settings.h"

#include "base64.h"
#include "global.h"
#include "logger.h"
#include "pathresolver.h"
#include "utils.h"
#include "xmlwriter.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMutex>
#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QSettings>
#include <QtCore/QSize>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <stdexcept>

using namespace Movida;

Q_GLOBAL_STATIC(QMutex, MvdSettingsLock)

/*!
    \class MvdSettings settings.h
    \ingroup MvdCore Singletons

    \brief The MvdSettings class allows to both handle application preferences
    at run time and to load/store them from/to a platform independent XML format.

    <b>Movida::settings()</b> can be used as a convenience method to access the singleton.

    The current version uses QSettings as a backend, with a custom format to
    read and write from/to xml files.

    Please refer to MvdPathResolver::settingsDir() for the exact location
    of the settings file on each platform.

    Each setting consists of a QString that specifies the setting's name (the key)
    and a QVariant that stores the data associated with the key.
    To write a setting, use setValue().

    For example:
    \verbatim
    Movida::settings().setValue("plugins/blue/max-results", 20);
    \endverbatim

    If there already exists a setting with the same key, the existing value is overwritten
    by the new value. For efficiency, the changes may not be saved to permanent storage immediately.

    You can get a setting's value back using value():
    \verbatim
    int maxResults = Movida::settings().value("plugins/blue/max-results").toInt();
    \endverbatim

    If there is no setting with the specified name, MvdSettings returns a null QVariant
    (which can be converted to the integer 0). You can specify another default value by
    passing a second argument to value().

    To test whether a given key exists, call contains().
    To remove the setting associated with a key, call remove().
    To remove all keys, call clear().

    <b>QVariant and GUI Types</b>

    Because QVariant is part of the mvdcore library, it cannot provide conversion functions to data types
    such as QColor, QImage, and QPixmap, which are part of QtGui.

    In other words, there is no toColor(), toImage(), or toPixmap() functions in QVariant.

    Instead, you can use the QVariant::value() or the qVariantValue() template function. For example:
    \verbatim
    QColor color = Movida::settings().value("plugins/blue/myfavouritecolor").value<QColor>();
    \endverbatim

    The inverse conversion (e.g., from QColor to QVariant) is automatic for all data types supported by
    QVariant, including GUI-related types:
    \verbatim
    Movida::settings().setValue("plugins/blue/myfavouritecolor", QColor("blue"));
    \endverbatim

    Custom types registered using qRegisterMetaType() and qRegisterMetaTypeStreamOperators()
    can be stored using MvdSettings.

    <b>Key Syntax</b>

    Setting keys can contain any Unicode characters and are <b>case sensitive</b>.
    Do not use slashes ('/' and '\') in key names; They have a special meaning.
    You can form hierarchical keys using the '/' character as a separator, similar to Unix file paths. For example:
    \verbatim
    Movida::settings().setValue("plugins/blue/color", QColor("blue"));
    Movida::settings().setValue("plugins/blue/number", 42);
    Movida::settings().setValue("plugins/blue/42", "Answer to Life, the Universe, and Everything");
    \endverbatim

    If you want to save or restore many settings with the same prefix, you can specify the prefix using
    beginGroup() and call endGroup() at the end. Here's the same example again, but this time using the
    group mechanism:

    \verbatim
    Movida::settings().beginGroup("plugins/blue");
    Movida::settings().setValue("color", QColor("blue"));
    Movida::settings().setValue("number", 42);
    Movida::settings().setValue("42", "Answer to Life, the Universe, and Everything");
    Movida::settings().endGroup();
    \endverbatim

    If a group is set using beginGroup(), the behavior of most functions changes consequently.
    Groups can be set recursively.
    In addition to groups, QSettings also supports an "array" concept.
    See beginReadArray() and beginWriteArray() for details.
*/

/************************************************************************

 *************************************************************************/

namespace {
struct SettingsGroup;
typedef QMap< QString, QVariant > SettingsList;
typedef QMap< QString, SettingsGroup > SettingsGroupList;

struct SettingsGroup {
    SettingsGroupList children;
    SettingsList map;
};

//! \internal Recursively writes a group of settings to an xml file.
void writeSettingsNode(MvdXmlWriter &xml, const SettingsGroup &node, const QString &groupName = QString())
{
    if (!groupName.isEmpty())
        xml.writeOpenTag("group", MvdAttribute("name", groupName));

    for (SettingsGroupList::ConstIterator it = node.children.constBegin(); it != node.children.constEnd(); ++it)
        writeSettingsNode(xml, it.value(), it.key());

    for (SettingsList::ConstIterator it = node.map.constBegin(); it != node.map.constEnd(); ++it) {
        xml.writeTaggedString(
            "setting",
            variantToString(it.value()),
            MvdAttribute("name", it.key()));
    }

    if (!groupName.isEmpty())
        xml.writeCloseTag("group");
}

//! \internal Recursively parses an xml node looking for <group> and <setting> tags. \p node is the root node.
void parseXmlNode(xmlDocPtr doc, xmlNodePtr node, QSettings::SettingsMap *map, QString path)
{
    Q_ASSERT(doc && node && map);

    node = node->children;
    while (node) {
        if (node->type != XML_ELEMENT_NODE) {
            node = node->next;
            continue;
        }

        QString name;

        xmlChar *attr = xmlGetProp(node, (const xmlChar *)"name");
        if (attr) {
            name = QString((const char *)attr).trimmed();
            xmlFree(attr);
        }

        if (name.isEmpty()) {
            node = node->next;
            continue;
        }

        // Skip "group" nodes with no children (IOW with no settings!)
        if (!xmlStrcmp(node->name, (const xmlChar *)"group") && node->children) {
            QString p = path.isEmpty() ? name : QString(path).append("/").append(name);
            parseXmlNode(doc, node, map, p);
        } else if (!xmlStrcmp(node->name, (const xmlChar *)"setting")) {
            QString s((const char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1));
            QVariant v = stringToVariant(s.trimmed());
            if (v.isValid()) {
                if (!path.isEmpty())
                    map->insert(QString(path).append("/").append(name), v);
                else map->insert(name, v);

                //if (!path.isEmpty())
                //	qDebug("Loading key %s", QString(path).append("/").append(name).toLatin1().constData());
                //else qDebug("Loading key %s", name.toLatin1().constData());
            }
        }

        node = node->next;
    }
}

//! Reads a movida-settings xml file into a QSettings map.
bool readXmlSettings(QIODevice &device, QSettings::SettingsMap &map)
{
    if (device.size() > 1024 * 1024) {
        eLog() << "::readXmlSettings: Settings file too big: " << device.size();
        return false;
    }

    QByteArray data = device.readAll();
    xmlDocPtr doc = xmlParseMemory(data.constData(), data.size());

    if (!doc) {
        eLog() << "::readXmlSettings: Failed to load settings.";
        return false;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (xmlStrcmp(node->name, (const xmlChar *)"movida-settings")) {
        eLog() << "::readXmlSettings: Not a valid movida settings file.";
        return false;
    }

    // Map an alphabetically ordered tree structure to path-like keys
    //
    // <group name='movida'>
    //   <group name='movie-list'>
    //     <setting name='sort-column'></setting>
    //   </group>
    // </group>
    //
    // --> "movida/movie-list/sort-column"
    parseXmlNode(doc, node, &map, QString());
    xmlFreeDoc(doc);

    return true;
}

//! Writes a QSettings map to a movida-settings xml file.
bool writeXmlSettings(QIODevice &device, const QSettings::SettingsMap &map)
{
    // Map path-like keys to an alphabetically ordered tree structure
    //
    // i.e. "movida/movie-list/sort-column" -->
    //
    // <group name='movida'>
    //   <group name='movie-list'>
    //     <setting name='sort-column'></setting>
    //   </group>
    // </group>
    SettingsGroup root;

    // typedef QMap<QString, QVariant> SettingsMap
    for (QSettings::SettingsMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it) {
        const QString &key = it.key();
        const QVariant &value = it.value();

        // qDebug("Writing key %s", key.toLatin1().constData());

        QStringList path = key.split("/");
        SettingsGroup *node = &root;

        for (int i = 0; i < path.size(); ++i) {
            const QString &k = path.at(i);

            // k is the setting name
            if (i == path.size() - 1)
                node->map.insert(k, value);
            else {
                SettingsGroupList::Iterator it = node->children.find(k);
                if (it == node->children.end()) {
                    SettingsGroup newNode;
                    node = &(node->children.insert(k, newNode).value());
                } else node = &(it.value());
            }
        }
    }

    MvdXmlWriter xml(&device);
    QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    xml.writeOpenTag("movida-settings", MvdAttribute("update", dateTime));

    writeSettingsNode(xml, root);

    xml.writeCloseTag("movida-settings");
    return true;
}

} // anonymous namespace


/************************************************************************
    MvdSettings
 *************************************************************************/

//! \internal
volatile MvdSettings *MvdSettings::mInstance = 0;
bool MvdSettings::mDestroyed = false;

//! \internal Private constructor.
MvdSettings::MvdSettings()
{
    QSettings::Format xmlFormat = QSettings::registerFormat("xml",
        ::readXmlSettings, ::writeXmlSettings);

    QSettings::setPath(xmlFormat, QSettings::UserScope, Movida::paths().settingsDir());

#if defined (Q_WS_WIN)
    mSettings = new QSettings(xmlFormat, QSettings::UserScope,
        QCoreApplication::organizationName(), QCoreApplication::applicationName());
#else
    // QSettings appends "{Org}/{App}.xml" so we need a hack to store settings in $HOME/.{Org}!
    mSettings = new QSettings(xmlFormat, QSettings::UserScope,
        QCoreApplication::organizationName().prepend("."), QCoreApplication::applicationName());
#endif
}

/*!
    Returns the application unique settings manager.
*/
MvdSettings &MvdSettings::instance()
{
    if (!mInstance) {
        QMutexLocker locker(MvdSettingsLock());
        if (!mInstance) {
            if (mDestroyed) throw std::runtime_error("Settings: access to dead reference");
            create();
        }
    }

    return (MvdSettings &) * mInstance;
}

//! Destructor.
MvdSettings::~MvdSettings()
{
    delete mSettings;
    mInstance = 0;
    mDestroyed = true;
}

void MvdSettings::create()
{
    // Local static members are instantiated as soon
    // as this function is entered for the first time
    // (Scott Meyers singleton)
    static MvdSettings instance;

    mInstance = &instance;
}

/*!
    Convenience method to access the MvdSettings singleton.
*/
MvdSettings &Movida::settings()
{
    return MvdSettings::instance();
}

/*!
    Removes all entries in the primary location associated to this settings object.
    Entries in fallback locations are not removed.
    If you only want to remove the entries in the current group(), use remove("") instead.
*/
void MvdSettings::clear()
{
    mSettings->clear();
}

/*!
    Appends prefix to the current group.
    Groups are useful to avoid typing in the same setting paths over and over.
*/
void MvdSettings::beginGroup(const QString &prefix)
{
    mSettings->beginGroup(prefix);
}

//! Resets the group to what it was before the corresponding beginGroup() call.
void MvdSettings::endGroup()
{
    mSettings->endGroup();
}

//! Returns the current group as set by beginGroup().
QString MvdSettings::group() const
{
    return mSettings->group();
}

/*!
    Adds prefix to the current group and starts reading from an array. Returns the size of the array.
    Use beginWriteArray() to write the array in the first place.
*/
int MvdSettings::beginReadArray(const QString &prefix)
{
    return mSettings->beginReadArray(prefix);
}

/*!
    Adds prefix to the current group and starts writing an array of size size.
    If size is -1 (the default), it is automatically determined based on the indexes of the entries written.
    If you have many occurrences of a certain set of keys, you can use arrays to make your life easier.
*/
void MvdSettings::beginWriteArray(const QString &prefix, int size)
{
    mSettings->beginWriteArray(prefix, size);
}

/*! Sets the current array index to i. Calls to functions such as setValue(), value(), remove(),
    and contains() will operate on the array entry at that index.
    You must call beginReadArray() or beginWriteArray() before you can call this function.
*/
void MvdSettings::setArrayIndex(int i)
{
    mSettings->setArrayIndex(i);
}

//! Closes the array that was started using beginReadArray() or beginWriteArray().
void MvdSettings::endArray()
{
    mSettings->endArray();
}

/*! Returns true if there exists a setting called key; returns false otherwise.
    If a group is set using beginGroup(), key is taken to be relative to that group.
*/
bool MvdSettings::contains(const QString &key) const
{
    return mSettings->contains(key);
}

//! Removes the setting key and any sub-settings of key.
void MvdSettings::remove(const QString &key)
{
    mSettings->remove(key);
}

/*! Sets the value of setting key to value but only if the key does not exist already.
*/
void MvdSettings::setDefaultValue(const QString &key, const QVariant &value)
{
    if (contains(key))
        return;
    mSettings->setValue(key, value);
}

/*! Sets the value of setting key to value.
    If the key already exists, the previous value is overwritten.
*/
void MvdSettings::setValue(const QString &key, const QVariant &value)
{
    mSettings->setValue(key, value);
}

/*! Returns the value for setting key. If the setting doesn't exist, returns defaultValue.
    If no default value is specified, a default QVariant is returned.
*/
QVariant MvdSettings::value(const QString &key, const QVariant &defaultValue) const
{
    if (!contains(key))
        qDebug("MvdSettings: key not found: %s", key.toLatin1().constData());
    return mSettings->value(key, defaultValue);
}

//! Returns a status code indicating the first error that was met, or NoError if no error occurred.
MvdSettings::Status MvdSettings::status() const
{
    QSettings::Status s = mSettings->status();

    switch (s) {
        case QSettings::AccessError:
            return AccessError;

        case QSettings::FormatError:
            return FormatError;

        default:
            ;
    }
    return NoError;
}
