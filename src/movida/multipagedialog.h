/**************************************************************************
** Filename: multipagedialog.h
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

#ifndef MVD_MULTIPAGEDIALOG_H
#define MVD_MULTIPAGEDIALOG_H

#include "ui_multipagedialog.h"
#include <QDialog>
#include <QMap>
#include <QList>

class MvdMPDialogPage;
class QShowEvent;

class MvdMultiPageDialog : public QDialog, private Ui::MvdMultiPageDialog
{
	Q_OBJECT

public:
	MvdMultiPageDialog(QWidget* parent = 0);

	virtual int addPage(MvdMPDialogPage* p);
	virtual void showPage(MvdMPDialogPage* p);

	virtual MvdMPDialogPage* currentPage() const;
	virtual int currentIndex() const;

	virtual QDialogButtonBox* buttonBox() const;

	void setSubtitle(const QString& s);
	QString subtitle() const;

	virtual void setAdvancedControlsVisible(bool visible);
	virtual bool advancedControlsVisible() const;

	virtual int addAdvancedControl(const QString& text, bool enabled = true);

	virtual void setAdvancedControlEnabled(int control, bool enabled = true);
	virtual bool advancedControlEnabled(int control);

	virtual void advancedControlHandler(int control);

protected:
	virtual void showEvent(QShowEvent* event);

protected slots:
	virtual void validationStateChanged(MvdMPDialogPage* page);
	virtual void modifiedStateChanged(MvdMPDialogPage* page);

signals:
	void externalActionTriggered(const QString& id, const QVariant& data
		= QVariant());
	void advancedControlTriggered(int id);
	void currentPageChanged(MvdMPDialogPage* page);

private slots:
	void do_validationStateChanged(MvdMPDialogPage* p = 0);
	void do_modifiedStateChanged(MvdMPDialogPage* p = 0);
	void linkActivated(const QString& url);
	void emit_currentPageChanged();

private:
	struct AdvancedControl
	{
		AdvancedControl(int aid = -1) : id(aid) {}
		AdvancedControl(int aid, const QString& alabel, bool aenabled = true)
			: id(aid), label(alabel), enabled(aenabled) {}

		bool operator==(const AdvancedControl& o) const { return id == o.id; };

		QString label;
		int id;
		bool enabled;
	};

	inline void layoutAdvancedControls();

	QString mSubtitle;
	QList<AdvancedControl> mAdvanced;
	int mIds;
};

#endif // MVD_MULTIPAGEDIALOG_H
