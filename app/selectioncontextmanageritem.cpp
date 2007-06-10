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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "selectioncontextmanageritem.moc"

// Qt
#include <QLabel>

// KDE
#include <kfileitem.h>
#include <kglobal.h>
#include <klocale.h>

// Local
#include "sidebar.h"
#include <lib/imageviewpart.h>
#include <lib/document.h>
#include <lib/documentfactory.h>

namespace Gwenview {

struct SelectionContextManagerItemPrivate {
	SideBarGroup* mGroup;

	QWidget* mOneFileWidget;
	QLabel* mOneFileImageLabel;
	QLabel* mOneFileTextLabel;
	QLabel* mMultipleFilesLabel;
	ImageViewPart* mImageView;
	Document::Ptr mDocument;
};

SelectionContextManagerItem::SelectionContextManagerItem()
: AbstractContextManagerItem()
, d(new SelectionContextManagerItemPrivate) {
	d->mImageView = 0;
}

SelectionContextManagerItem::~SelectionContextManagerItem() {
	delete d;
}

void SelectionContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mOneFileWidget = new QWidget();

	d->mOneFileImageLabel = new QLabel(d->mOneFileWidget);

	d->mOneFileTextLabel = new QLabel(d->mOneFileWidget);
	d->mOneFileTextLabel->setWordWrap(true);

	QVBoxLayout* layout = new QVBoxLayout(d->mOneFileWidget);
	layout->setMargin(0);
	layout->addWidget(d->mOneFileImageLabel);
	layout->addWidget(d->mOneFileTextLabel);

	d->mMultipleFilesLabel = new QLabel();

	d->mGroup = sideBar->createGroup(i18n("Selection"));
	d->mGroup->addWidget(d->mOneFileWidget);
	d->mGroup->addWidget(d->mMultipleFilesLabel);

	d->mGroup->hide();
}

void SelectionContextManagerItem::updateSideBar(const KFileItemList& itemList) {
	if (itemList.count() == 0) {
		d->mGroup->hide();
		// "Garbage collect" document
		d->mDocument = 0;
		return;
	}

	d->mGroup->show();
	if (itemList.count() == 1) {
		fillOneFileGroup(itemList.first());
		return;
	}

	fillMultipleItemsGroup(itemList);
}

void SelectionContextManagerItem::fillOneFileGroup(const KFileItem* item) {
	QString fileSize = KGlobal::locale()->formatByteSize(item->size());
	d->mOneFileTextLabel->setText(
		i18n("%1\n%2\n%3", item->name(), item->timeString(), fileSize)
		);

	d->mOneFileWidget->show();
	d->mMultipleFilesLabel->hide();

	if (item->isDir()) {
		d->mOneFileImageLabel->hide();
	} else {
		d->mDocument = DocumentFactory::instance()->load(item->url());
		connect(d->mDocument.data(), SIGNAL(loaded()), SLOT(updatePreview()) );
	}
}

void SelectionContextManagerItem::fillMultipleItemsGroup(const KFileItemList& itemList) {
	// "Garbage collect" document
	d->mDocument = 0;

	int folderCount = 0, fileCount = 0;
	Q_FOREACH(KFileItem* item, itemList) {
		if (item->isDir()) {
			folderCount++;
		} else {
			fileCount++;
		}
	}

	if (folderCount == 0) {
		d->mMultipleFilesLabel->setText(i18n("%1 files selected", fileCount));
	} else if (fileCount == 0) {
		d->mMultipleFilesLabel->setText(i18n("%1 folders selected", folderCount));
	} else {
		d->mMultipleFilesLabel->setText(i18n("%1 folders and %2 files selected", folderCount, fileCount));
	}
	d->mOneFileWidget->hide();
	d->mMultipleFilesLabel->show();
}

void SelectionContextManagerItem::setImageView(ImageViewPart* imageView) {
	if (d->mImageView) {
		disconnect(d->mImageView, 0, this, 0);
	}

	d->mImageView = imageView;
}

void SelectionContextManagerItem::updatePreview() {
	Q_ASSERT(d->mDocument);
	if (!d->mDocument) {
		return;
	}

	QImage image = d->mDocument->image().scaled(160, 160, Qt::KeepAspectRatio);
	d->mOneFileImageLabel->setPixmap(QPixmap::fromImage(image));
	d->mOneFileImageLabel->show();
}

} // namespace
