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
// Self
#include "bookmarkdialog.moc"

// Qt
#include <qlabel.h>

// KDE
#include <kfile.h>
#include <kicondialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlrequester.h>

// Local
#include "bookmark.h"
#include "bookmarkdialogbase.h"

namespace Gwenview {

struct BookmarkDialog::Private {
	BookmarkDialogBase* mContent;
	Bookmark* mBookmark;
};

BookmarkDialog::BookmarkDialog(QWidget* parent, Bookmark* bookmark)
: KDialogBase(parent,"folderconfig",true,QString::null,Ok|Cancel)
{
	d=new Private;
	d->mContent=new BookmarkDialogBase(this);
	d->mBookmark=bookmark;

	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	d->mContent->mUrl->setMode(KFile::Directory);
	d->mContent->mUrl->setURL(bookmark->url().prettyURL());
	d->mContent->mIcon->setIcon(bookmark->icon());
	d->mContent->mTitle->setText(bookmark->title());
	
	connect(d->mContent->mTitle,SIGNAL(textChanged(const QString&)),
		this, SLOT(updateOk()));
	connect(d->mContent->mIcon,SIGNAL(iconChanged(QString)),
		this, SLOT(updateOk()));
	
	connect(d->mContent->mUrl,SIGNAL(textChanged(const QString&)),
		this, SLOT(updateOk()));

	setCaption( i18n("Add/Edit Bookmark") );
	updateOk();
}

BookmarkDialog::~BookmarkDialog() {
	delete d;
}

void BookmarkDialog::updateOk() {
	bool enabled=
		!d->mContent->mTitle->text().isEmpty()
		&& !d->mContent->mUrl->url().isEmpty();

	enableButton(Ok, enabled);
}


} // namespace
