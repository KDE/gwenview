/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QPainter>
#include <QPair>
#include <QPushButton>
#include <QVBoxLayout>

// KDE
#include <kfileitem.h>
#include <klocale.h>
#include <kwordwrap.h>

// Local
#include "contextmanager.h"
#include "imagemetainfodialog.h"
#include "sidebar.h"
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfo.h>
#include <lib/imageviewpart.h>
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif


/**
 * This widget is capable of showing multiple lines of key/value pairs. It
 * fades out the value if it does not fit the available width.
 */
class KeyValueWidget : public QWidget {
public:
	KeyValueWidget(QWidget* parent)
	: QWidget(parent) {}

	void addItem(const QString& key, const QString& value) {
		mList << ListItem(key, value);
	}

	void clear() {
		mList.clear();
	}

	virtual QSize sizeHint() const {
		int height = fontMetrics().height() * mList.size();
		return QSize(-1, height);
	}

	virtual QSize minimumSizeHint() const {
		return sizeHint();
	}

protected:
	virtual void paintEvent(QPaintEvent*) {
		QPainter painter(this);
		int posY = fontMetrics().ascent();
		int lineHeight = fontMetrics().height();
		int maxWidth = width();

		QFont keyFont(font());
		keyFont.setBold(true);
		QFontMetrics keyFM(keyFont, this);

		QRegExp re("<b>([^<]+)</b>(.*)");
		Q_FOREACH(ListItem item, mList) {
			QString key = item.first;
			QString value = item.second;

			// FIXME: KDE4.1: Remove this when string freeze is lifted
			QString tmp = i18n("<b>%1:</b> %2", key, value);
			int pos = re.indexIn(tmp);
			if (pos > -1) {
				key = re.cap(1);
				value = re.cap(2);
			} else {
				key += ':';
			}

			painter.save();
			painter.setFont(keyFont);
			painter.drawText(0, posY, key);
			int posX = keyFM.width(key);

			painter.setFont(font());
			KWordWrap::drawFadeoutText(&painter, posX, posY, maxWidth - posX, value);
			posY += lineHeight;
			painter.restore();
		}
	}

private:
	typedef QPair<QString, QString> ListItem;
	QList<ListItem> mList;
};


struct InfoContextManagerItemPrivate {
	SideBar* mSideBar;
	SideBarGroup* mGroup;

	QWidget* mOneFileWidget;
	KeyValueWidget* mKeyValueWidget;
	QLabel* mMultipleFilesLabel;
	KFileItem mFileItem;
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

	QStringList list = GwenviewConfig::preferredMetaInfoKeyList();
	connect(&d->mImageMetaInfo, SIGNAL(preferredMetaInfoKeyListChanged(const QStringList&)),
		SLOT(updateOneFileInfo()) );
	d->mImageMetaInfo.setPreferredMetaInfoKeyList(list);
}

InfoContextManagerItem::~InfoContextManagerItem() {
	QStringList list = d->mImageMetaInfo.preferredMetaInfoKeyList();
	GwenviewConfig::setPreferredMetaInfoKeyList(list);
	GwenviewConfig::self()->writeConfig();
	delete d;
}


void InfoContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );
	d->mOneFileWidget = new QWidget();

	d->mKeyValueWidget = new KeyValueWidget(d->mOneFileWidget);

	QLabel* moreLabel = new QLabel(d->mOneFileWidget);
	moreLabel->setText(QString("<a href='#'>%1</a>").arg(i18n("More...")));
	moreLabel->setAlignment(Qt::AlignRight);

	QVBoxLayout* layout = new QVBoxLayout(d->mOneFileWidget);
	layout->setMargin(2);
	layout->setSpacing(2);
	layout->addWidget(d->mKeyValueWidget);
	layout->addWidget(moreLabel);

	d->mMultipleFilesLabel = new QLabel();

	d->mGroup = sideBar->createGroup(i18n("Information"));
	d->mGroup->addWidget(d->mOneFileWidget);
	d->mGroup->addWidget(d->mMultipleFilesLabel);

	d->mGroup->hide();

	connect(moreLabel, SIGNAL(linkActivated(const QString&)), SLOT(showMetaInfoDialog()) );
}


void InfoContextManagerItem::updateSideBarContent() {
	LOG("updateSideBarContent");
	if (!d->mSideBar->isVisible()) {
		LOG("updateSideBarContent: not visible, not updating");
		return;
	}
	LOG("updateSideBarContent: really updating");

	KFileItemList itemList = contextManager()->selection();
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

	d->mImageMetaInfo.setFileItem(d->mFileItem);
	d->mImageMetaInfo.setImageSize(QSize());
	d->mImageMetaInfo.setExiv2Image(0);
	updateOneFileInfo();
	if (!item.isDir()) {
		d->mDocument = DocumentFactory::instance()->load(item.url());
		connect(d->mDocument.data(), SIGNAL(metaDataLoaded()),
			SLOT(slotMetaDataLoaded()) );

		if (d->mDocument->isMetaDataLoaded()) {
			slotMetaDataLoaded();
		}
	}
}

void InfoContextManagerItem::fillMultipleItemsGroup(const KFileItemList& itemList) {
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
	// We might not have a document anymore if we just selected two files
	if (!d->mDocument) {
		return;
	}
	d->mImageMetaInfo.setImageSize(d->mDocument->size());
	d->mImageMetaInfo.setExiv2Image(d->mDocument->exiv2Image());
	updateOneFileInfo();
}


void InfoContextManagerItem::updateOneFileInfo() {
	if (!d->mSideBar) {
		// Not initialized yet
		return;
	}
	QStringList list;
	d->mKeyValueWidget->clear();
	Q_FOREACH(QString key, d->mImageMetaInfo.preferredMetaInfoKeyList()) {
		QString label;
		QString value;
		d->mImageMetaInfo.getInfoForKey(key, &label, &value);

		if (!label.isEmpty() && !value.isEmpty()) {
			d->mKeyValueWidget->addItem(label, value);
		}
	}

	// FIXME: This should be handled by mKeyValueWidget: it should schedule a
	// delayed update when clear() and addItem() are called.
	d->mKeyValueWidget->updateGeometry();
	d->mKeyValueWidget->update();

	d->mKeyValueWidget->show();
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
