// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// Qt
#include <qlabel.h>

// KDE
#include <kfile.h>
#include <kicondialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlrequester.h>

// Local
#include "branchpropertiesdialog.h"
#include "branchpropertiesdialogbase.h"
#include "branchpropertiesdialog.moc"
namespace Gwenview {

class BranchPropertiesDialogPrivate {
public:
	BranchPropertiesDialogBase* mContent;
	BranchPropertiesDialog::Mode mMode;
};

BranchPropertiesDialog::BranchPropertiesDialog(QWidget* parent, BranchPropertiesDialog::Mode mode)
: KDialogBase(parent,"folderconfig",true,QString::null,Ok|Cancel)
{
	d=new BranchPropertiesDialogPrivate;
	d->mContent=new BranchPropertiesDialogBase(this);
	d->mMode=mode;

	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	d->mContent->mUrl->setMode(KFile::Directory);
	d->mContent->mIcon->setIcon("folder");
	
	connect(d->mContent->mTitle,SIGNAL(textChanged(const QString&)),
		this, SLOT(updateOk()));
	connect(d->mContent->mIcon,SIGNAL(iconChanged(QString)),
		this, SLOT(updateOk()));
	
	if (mode==BOOKMARK_GROUP) {
		d->mContent->mUrlLabel->hide();
		d->mContent->mUrl->hide();
	} else {
		connect(d->mContent->mUrl,SIGNAL(textChanged(const QString&)),
			this, SLOT(updateOk()));
	}

	updateOk();
}

BranchPropertiesDialog::~BranchPropertiesDialog() {
	delete d;
}

void BranchPropertiesDialog::updateOk() {
	bool enabled=
		!d->mContent->mTitle->text().isEmpty()
		&& (d->mMode==BOOKMARK_GROUP || !d->mContent->mUrl->url().isEmpty());

	enableButton(Ok, enabled);
}

void BranchPropertiesDialog::setIcon(const QString& icon) {
	d->mContent->mIcon->setIcon(icon);
}

QString BranchPropertiesDialog::icon() const {
	return d->mContent->mIcon->icon();
}

void BranchPropertiesDialog::setTitle(const QString& title) {
	d->mContent->mTitle->setText(title);
}

QString BranchPropertiesDialog::title() const {
	return d->mContent->mTitle->text();
}

void BranchPropertiesDialog::setURL(const QString& url) {
	d->mContent->mUrl->setURL(url);
}

QString BranchPropertiesDialog::url() const {
	return d->mContent->mUrl->url();
}

} // namespace
