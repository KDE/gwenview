// vi: tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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

#include "thumbnaildetailsdialog.moc"

// Qt
#include <qcheckbox.h>

// Local
#include "filethumbnailview.h"
#include "thumbnaildetailsdialogbase.h"


namespace Gwenview {

struct ThumbnailDetailsDialog::Private {
	FileThumbnailView* mView;
	ThumbnailDetailsDialogBase* mContent;
};
	
ThumbnailDetailsDialog::ThumbnailDetailsDialog(FileThumbnailView* view)
: KDialogBase(
	view, 0, false /* modal */, QString::null, KDialogBase::Close,
	KDialogBase::Close, true /* separator */)
, d(new ThumbnailDetailsDialog::Private)
{
	d->mView=view;
	d->mContent=new ThumbnailDetailsDialogBase(this);
	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	
	int details=d->mView->itemDetails();
	d->mContent->mShowFileName->setChecked(details & FileThumbnailView::FILENAME);
	d->mContent->mShowFileDate->setChecked(details & FileThumbnailView::FILEDATE);
	d->mContent->mShowFileSize->setChecked(details & FileThumbnailView::FILESIZE);
	d->mContent->mShowImageSize->setChecked(details & FileThumbnailView::IMAGESIZE);

	connect(d->mContent->mShowFileName, SIGNAL(toggled(bool)), SLOT(applyChanges()) );
	connect(d->mContent->mShowFileDate, SIGNAL(toggled(bool)), SLOT(applyChanges()) );
	connect(d->mContent->mShowFileSize, SIGNAL(toggled(bool)), SLOT(applyChanges()) );
	connect(d->mContent->mShowImageSize, SIGNAL(toggled(bool)), SLOT(applyChanges()) );
}

ThumbnailDetailsDialog::~ThumbnailDetailsDialog() {
	delete d;
}


void ThumbnailDetailsDialog::applyChanges() {
	int details=
		(d->mContent->mShowFileName->isChecked() ? FileThumbnailView::FILENAME : 0)
		| (d->mContent->mShowFileDate->isChecked() ? FileThumbnailView::FILEDATE : 0)
		| (d->mContent->mShowFileSize->isChecked() ? FileThumbnailView::FILESIZE : 0)
		| (d->mContent->mShowImageSize->isChecked() ? FileThumbnailView::IMAGESIZE : 0)
		;
	d->mView->setItemDetails(details);
}


} // namespace
