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

#include <exiv2/exif.hpp>

// Qt
#include <QLabel>

// KDE
#include <kfileitem.h>
#include <kglobal.h>
#include <klocale.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"
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


static void addExifValue(QStringList& list, const Exiv2::ExifData& exifData, const char* keyName) {
	Exiv2::ExifKey key(keyName);
	Exiv2::ExifData::const_iterator it = exifData.findKey(key);

	if (it == exifData.end()) {
		return;
	}

	QString label = QString::fromUtf8(it->tagLabel().c_str());
	std::ostringstream stream;
	stream << *it;
	QString value = QString::fromUtf8(stream.str().c_str());
	list << i18n("%1: %2", label, value);
}


struct InfoContextManagerItemPrivate {
	SideBar* mSideBar;
	SideBarGroup* mGroup;

	QWidget* mOneFileWidget;
	QLabel* mOneFileImageLabel;
	QLabel* mOneFileTextLabel;
	QLabel* mMultipleFilesLabel;
	KFileItem mFileItem;
	Document::Ptr mDocument;


	void addMetaDataLines(QStringList& list) {
		if (!mDocument) {
			return;
		}

		const Exiv2::Image* exiv2Image = mDocument->exiv2Image();
		if (!exiv2Image) {
			return;
		}

		if (!exiv2Image->supportsMetadata(Exiv2::mdExif)) {
			return;
		}
		const Exiv2::ExifData& exifData = exiv2Image->exifData();

		addExifValue(list, exifData, "Exif.Photo.ISOSpeedRatings");
		addExifValue(list, exifData, "Exif.Photo.ExposureTime");
		addExifValue(list, exifData, "Exif.Photo.Flash");
	}
};


InfoContextManagerItem::InfoContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new InfoContextManagerItemPrivate) {
	d->mSideBar = 0;
	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );
}

InfoContextManagerItem::~InfoContextManagerItem() {
	delete d;
}


void InfoContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );
	d->mOneFileWidget = new QWidget();

	d->mOneFileImageLabel = new QLabel(d->mOneFileWidget);

	d->mOneFileTextLabel = new QLabel(d->mOneFileWidget);
	d->mOneFileTextLabel->setWordWrap(true);

	QVBoxLayout* layout = new QVBoxLayout(d->mOneFileWidget);
	layout->setMargin(0);
	layout->addWidget(d->mOneFileImageLabel);
	layout->addWidget(d->mOneFileTextLabel);

	d->mMultipleFilesLabel = new QLabel();

	d->mGroup = sideBar->createGroup(i18n("Information"));
	d->mGroup->addWidget(d->mOneFileWidget);
	d->mGroup->addWidget(d->mMultipleFilesLabel);

	d->mGroup->hide();
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
	d->mFileItem = item;
	d->mOneFileWidget->show();
	d->mMultipleFilesLabel->hide();

	if (item.isDir()) {
		d->mOneFileImageLabel->hide();
	} else {
		d->mDocument = DocumentFactory::instance()->load(item.url());
		connect(d->mDocument.data(), SIGNAL(imageRectUpdated()),
			SLOT(updatePreview()) );
		connect(d->mDocument.data(), SIGNAL(loaded()),
			SLOT(updatePreview()) );
		connect(d->mDocument.data(), SIGNAL(metaDataLoaded()),
			SLOT(updateOneFileInfo()) );
		// If it's already loaded, trigger updatePreview ourself
		if (d->mDocument->isLoaded()) {
			updatePreview();
		}
	}
	updateOneFileInfo();
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


void InfoContextManagerItem::updatePreview() {
	Q_ASSERT(d->mDocument);
	if (!d->mDocument) {
		return;
	}

	QImage image = d->mDocument->image().scaled(160, 160, Qt::KeepAspectRatio);
	d->mOneFileImageLabel->setPixmap(QPixmap::fromImage(image));
	d->mOneFileImageLabel->show();
}


void InfoContextManagerItem::updateOneFileInfo() {
	QString fileSize = KGlobal::locale()->formatByteSize(d->mFileItem.size());
	QStringList list;
	list << d->mFileItem.name()
		<< d->mFileItem.timeString()
		<< fileSize;

	d->addMetaDataLines(list);

	d->mOneFileTextLabel->setText(list.join("\n"));
	d->mOneFileTextLabel->show();
}


} // namespace
