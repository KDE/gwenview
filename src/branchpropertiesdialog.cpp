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

class BranchPropertiesDialogPrivate {
public:

	BranchPropertiesDialogBase* mContent;
};

BranchPropertiesDialog::BranchPropertiesDialog(QWidget* parent)
: KDialogBase(parent,"folderconfig",true,QString::null,Ok|Cancel)
{
	d=new BranchPropertiesDialogPrivate;
	d->mContent=new BranchPropertiesDialogBase(this);
	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	d->mContent->mUrl->setMode(KFile::Directory);
	d->mContent->mIcon->setIcon("folder");
	enableButton(Ok, false);
}

BranchPropertiesDialog::~BranchPropertiesDialog() {
	delete d;
}

void BranchPropertiesDialog::setContents(const QString& icon, const QString& title, const QString& url) {
	d->mContent->mTitle->setText(title);
	d->mContent->mUrl->setURL(url);
	d->mContent->mIcon->setIcon(icon);
	setCaption(i18n("Edit Branch"));
}

int BranchPropertiesDialog::exec() {
	connect(d->mContent->mTitle,SIGNAL(textChanged(const QString&)),
		this,SLOT(enableOk()));
	connect(d->mContent->mUrl,SIGNAL(textChanged(const QString&)),
		this,SLOT(enableOk()));
	connect(d->mContent->mIcon,SIGNAL(iconChanged(QString)),
		this,SLOT(enableOk()));
	return KDialog::exec();
}

void BranchPropertiesDialog::enableOk() {
	enableButton(Ok, true);
}

QString BranchPropertiesDialog::icon() {
	return d->mContent->mIcon->icon();
}

QString BranchPropertiesDialog::title() {
	return d->mContent->mTitle->text();
}

QString BranchPropertiesDialog::url() {
	return d->mContent->mUrl->url();
}
