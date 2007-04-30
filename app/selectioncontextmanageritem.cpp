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
#include <klocale.h>
#include <kfileitem.h>

// Local
#include "sidebar.h"
#include <lib/imageviewpart.h>
#include <lib/document.h>

namespace Gwenview {

SelectionContextManagerItem::SelectionContextManagerItem()
: AbstractContextManagerItem()
, mImageView(0) {
}

void SelectionContextManagerItem::setSideBar(SideBar* sideBar) {
	mOneFileWidget = new QWidget();

	mOneFileImageLabel = new QLabel(mOneFileWidget);

	mOneFileTextLabel = new QLabel(mOneFileWidget);
	mOneFileTextLabel->setWordWrap(true);

	QVBoxLayout* layout = new QVBoxLayout(mOneFileWidget);
	layout->setMargin(0);
	layout->addWidget(mOneFileImageLabel);
	layout->addWidget(mOneFileTextLabel);

	mMultipleFilesLabel = new QLabel();

	mGroup = sideBar->createGroup(i18n("Selection"));
	mGroup->addWidget(mOneFileWidget);
	mGroup->addWidget(mMultipleFilesLabel);

	mGroup->hide();
}

void SelectionContextManagerItem::updateSideBar(const KFileItemList& itemList) {
	if (itemList.count() == 0) {
		mGroup->hide();
		return;
	}

	mGroup->show();
	if (itemList.count() == 1) {
		fillOneFileGroup(itemList.first());
		return;
	}

	fillMultipleItemsGroup(itemList);
}

void SelectionContextManagerItem::fillOneFileGroup(const KFileItem* item) {
	mOneFileTextLabel->setText(
		i18n("%1\n%2\n%3", item->name(), item->timeString(), item->size())
		);
	mOneFileWidget->show();
	mMultipleFilesLabel->hide();
}

void SelectionContextManagerItem::fillMultipleItemsGroup(const KFileItemList& itemList) {
	int folderCount = 0, fileCount = 0;
	Q_FOREACH(KFileItem* item, itemList) {
		if (item->isDir()) {
			folderCount++;
		} else {
			fileCount++;
		}
	}

	if (folderCount == 0) {
		mMultipleFilesLabel->setText(i18n("%1 files selected", fileCount));
	} else if (fileCount == 0) {
		mMultipleFilesLabel->setText(i18n("%1 folders selected", folderCount));
	} else {
		mMultipleFilesLabel->setText(i18n("%1 folders and %2 files selected", folderCount, fileCount));
	}
	mOneFileWidget->hide();
	mMultipleFilesLabel->show();
}

void SelectionContextManagerItem::setImageView(ImageViewPart* imageView) {
	if (mImageView) {
		disconnect(mImageView, 0, this, 0);
	}

	if (imageView) {
		mImageView = imageView;
		connect(imageView, SIGNAL(completed()), SLOT(updatePreview()));
	} else {
		mImageView = 0;
		mOneFileImageLabel->hide();
	}
}

void SelectionContextManagerItem::updatePreview() {
	Q_ASSERT(mImageView);
	if (!mImageView) {
		return;
	}

	Document::Ptr document = mImageView->document();
	Q_ASSERT(document);
	if (!document) {
		return;
	}

	QImage image = document->image().scaled(160, 160, Qt::KeepAspectRatio);
	mOneFileImageLabel->setPixmap(QPixmap::fromImage(image));
	mOneFileImageLabel->show();
}

} // namespace
