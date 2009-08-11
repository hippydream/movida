/**************************************************************************
** Filename: multipagedialog.h
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

#ifndef MVD_MULTIPAGEDIALOG_H
#define MVD_MULTIPAGEDIALOG_H

#include "ui_multipagedialog.h"

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtGui/QDialog>

class MvdMPDialogPage;
class QShowEvent;

class MvdMultiPageDialog : public QDialog, private Ui::MvdMultiPageDialog
{
    Q_OBJECT

public:
    MvdMultiPageDialog(QWidget *parent = 0);

    virtual int addPage(MvdMPDialogPage *p);
    virtual void showPage(MvdMPDialogPage *p);

    virtual MvdMPDialogPage *currentPage() const;
    virtual int currentIndex() const;

    virtual QDialogButtonBox *buttonBox() const;
    virtual QDialogButtonBox *advancedButtonBox() const;

    void setSubtitle(const QString &s);
    QString subtitle() const;

    virtual void setAdvancedControlsVisible(bool visible);
    virtual bool advancedControlsVisible() const;

    void setDiscardChanges(bool discard) { mDiscardChanges = discard; }

    bool discardChanges() const { return mDiscardChanges; }

protected:
    virtual void showEvent(QShowEvent *event);

protected slots:
    virtual void validationStateChanged(MvdMPDialogPage *page);
    virtual void modifiedStateChanged(MvdMPDialogPage *page);

signals:
    void externalActionTriggered(const QString &id, const QVariant &data
    = QVariant());
    void currentPageChanged(MvdMPDialogPage *page);

private slots:
    void do_validationStateChanged(MvdMPDialogPage *p = 0);
    void do_modifiedStateChanged(MvdMPDialogPage *p = 0);
    void emit_currentPageChanged();

private:
    QString mSubtitle;
    bool mDiscardChanges;
};

#endif // MVD_MULTIPAGEDIALOG_H
