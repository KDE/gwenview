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
#include "infocontextmanageritem.moc"

// Qt
#include <QLabel>
#include <QPushButton>

// KDE
#include <kfileitem.h>
#include <klocale.h>

// Local
#include "contextmanager.h"
#include "imagemetainfodialog.h"
#include "sidebar.h"
#include <lib/imagemetainfo.h>
#include <lib/imageviewpart.h>
#include <lib/document.h>
#include <lib/documentfactory.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif


struct InfoContextManagerItemPrivate {
	SideBar* mSideBar;
	SideBarGroup* mGroup;

	QWidget* mOneFileWidget;
	QLabel* mOneFileTextLabel;
	QLabel* mMultipleFilesLabel;
	Document::Ptr mDocument;
	ImageMetaInfo mImageMetaInfo;

	QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;
};


InfoContextManagerItem::InfoContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new InfoContextManagerItemPrivate) {
	d->mSideBar = 0;
	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );

	QStringList list;
	list << "KFileItem.Name"
		<< "KFileItem.Size"
		<< "KFileItem.Time"
		<< "Exif.Photo.ISOSpeedRatings"
		<< "Exif.Photo.ExposureTime"
		<< "Exif.Photo.Flash"
		;
	connect(&d->mImageMetaInfo, SIGNAL(preferedMetaInfoKeyListChanged(const QStringList&)),
		SLOT(updateOneFileInfo()) );
	d->mImageMetaInfo.setPreferedMetaInfoKeyList(list);
}

InfoContextManagerItem::~InfoContextManagerItem() {
	delete d;
}


void InfoContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );
	d->mOneFileWidget = new QWidget();

	d->mOneFileTextLabel = new QLabel(d->mOneFileWidget);
	d->mOneFileTextLabel->setWordWrap(true);

	QPushButton* moreButton = new QPushButton(d->mOneFileWidget);
	moreButton->setText(i18n("More..."));

	QVBoxLayout* layout = new QVBoxLayout(d->mOneFileWidget);
	layout->setMargin(0);
	layout->addWidget(d->mOneFileTextLabel);
	layout->addWidget(moreButton);

	d->mMultipleFilesLabel = new QLabel();

	d->mGroup = sideBar->createGroup(i18n("Information"));
	d->mGroup->addWidget(d->mOneFileWidget);
	d->mGroup->addWidget(d->mMultipleFilesLabel);

	d->mGroup->hide();

	connect(moreButton, SIGNAL(clicked()), SLOT(showMetaInfoDialog()) );
}


void InfoContextManagerItem::updateSideBarContent() {
	LOG("updateSideBarContent");
	if (!d->mSideBar->isVisible()) {
		LOG("updateSideBarContent: not visible, not updating");
		return;
	}
	LOG("updateSideBarContent: really updating");

	QList<KFileItem> itemList = contextManager()->selection();
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

void InfoContextManagerItem::fillOneFileGroup(const KFileItem& item) {
	d->mImageMetaInfo.setFileItem(item);
	d->mOneFileWidget->show();
	d->mMultipleFilesLabel->hide();

	if (!item.isDir()) {
		d->mDocument = DocumentFactory::instance()->load(item.url());
		connect(d->mDocument.data(), SIGNAL(metaDataLoaded()),
			SLOT(slotMetaDataLoaded()) );
	}
	// FIXME: slotMetaDataLoaded will be called twice
	slotMetaDataLoaded();
}

void InfoContextManagerItem::fillMultipleItemsGroup(const QList<KFileItem>& itemList) {
	// "Garbage collect" document
	d->mDocument = 0;

	int folderCount = 0, fileCount = 0;
	Q_FOREACH(KFileItem item, itemList) {
		if (item.isDir()) {
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


void InfoContextManagerItem::slotMetaDataLoaded() {
	if (d->mDocument) {
		d->mImageMetaInfo.setExiv2Image(d->mDocument->exiv2Image());
	} else {
		d->mImageMetaInfo.setExiv2Image(0);
	}
	updateOneFileInfo();
}


void InfoContextManagerItem::updateOneFileInfo() {
	if (!d->mSideBar) {
		// Not initialized yet
		return;
	}
	QStringList list;
	Q_FOREACH(QString key, d->mImageMetaInfo.preferedMetaInfoKeyList()) {
		QString label;
		QString value;
		d->mImageMetaInfo.getInfoForKey(key, &label, &value);

		if (!label.isEmpty() && !value.isEmpty()) {
			list.append(i18n("%1: %2", label, value));
		}
	}

	d->mOneFileTextLabel->setText(list.join("\n"));
	d->mOneFileTextLabel->show();
}


void InfoContextManagerItem::showMetaInfoDialog() {
	if (!d->mImageMetaInfoDialog) {
		d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mOneFileWidget);
		d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
		d->mImageMetaInfoDialog->setImageMetaInfo(&d->mImageMetaInfo);
	}
	d->mImageMetaInfoDialog->show();
}


} // namespace
