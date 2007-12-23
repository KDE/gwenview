// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "imagemetainfodialog.moc"

// Qt
#include <QTreeView>
#include <QHeaderView>
#include <QLayout>
// KDE
#include <klocale.h>

// Local
#include <lib/imagemetainfo.h>

namespace Gwenview {


struct ImageMetaInfoDialogPrivate {
	ImageMetaInfo* mInfo;
	QTreeView* mTreeView;
};


ImageMetaInfoDialog::ImageMetaInfoDialog(QWidget* parent)
: KDialog(parent)
, d(new ImageMetaInfoDialogPrivate) {
	d->mInfo = 0;
	d->mTreeView = new QTreeView(this);
	setMainWidget(d->mTreeView);
	setCaption(i18n("Meta Information"));
	setButtons(KDialog::Close);
}


ImageMetaInfoDialog::~ImageMetaInfoDialog() {
	delete d;
}


void ImageMetaInfoDialog::setImageMetaInfo(ImageMetaInfo* info) {
	d->mInfo = info;
	d->mTreeView->setModel(info);
	d->mTreeView->header()->resizeSection(0, sizeHint().width() / 2 - layout()->margin()*2);
	d->mTreeView->expandAll();
}


QSize ImageMetaInfoDialog::sizeHint() const {
	return QSize(400, 300);
}


} // namespace
