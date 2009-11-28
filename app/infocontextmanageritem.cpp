/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

// KDE
#include <kfileitem.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>

// Local
#include "contextmanager.h"
#include "imagemetainfodialog.h"
#include "sidebar.h"
#include <lib/archiveutils.h>
#include <lib/eventwatcher.h>
#include <lib/gwenviewconfig.h>
#include <lib/preferredimagemetainfomodel.h>
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
 * This widget is capable of showing multiple lines of key/value pairs.
 */
class KeyValueWidget : public QWidget {
public:
	KeyValueWidget(QWidget* parent)
	: QWidget(parent) {
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	}

	void addRow(const QString& key, const QString& value) {
		QString keyString = i18nc(
			"@item:intext %1 is a key, we append a colon to it. A value is displayed after",
			"%1:", key);
		mRows.append(Row(keyString, value));
		updateGeometry();
	}

	void clear() {
		mRows.clear();
		updateGeometry();
	}

	virtual QSize sizeHint() const {
		int height = fontMetrics().height() * mRows.count();
		return QSize(150, height);
	}

protected:
	virtual void paintEvent(QPaintEvent*) {
		// Compute max width
		int valuePosX = 0;
		int maxValuePosX = width() / 2;
		Q_FOREACH(const Row& row, mRows) {
			int keyWidth = fontMetrics().width(row.first);
			if (keyWidth > maxValuePosX) {
				valuePosX = maxValuePosX;
				break;
			}
			valuePosX = qMax(valuePosX, keyWidth);
		}

		// Draw
		QPainter painter(this);
		int posY = 0;
		int height = fontMetrics().height();
		Q_FOREACH(const Row& row, mRows) {
			QString key = row.first;
			QString value = row.second;
			painter.drawText(0, posY, valuePosX, height, Qt::AlignRight, key);
			painter.drawText(valuePosX, posY, width() - valuePosX, height, Qt::AlignLeft, ' ' + value);
			posY += height;
		}
	}

private:
	typedef QPair<QString, QString> Row;
	QList<Row> mRows;
};


struct InfoContextManagerItemPrivate {
	InfoContextManagerItem* q;
	SideBarGroup* mGroup;

	// One selection fields
	QWidget* mOneFileWidget;
	KeyValueWidget* mKeyValueWidget;
	KFileItem mFileItem;
	Document::Ptr mDocument;

	// Multiple selection fields
	QLabel* mMultipleFilesLabel;

	QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;

	void updateMetaInfoDialog() {
		if (!mImageMetaInfoDialog) {
			return;
		}
		ImageMetaInfoModel* model = mDocument ? mDocument->metaInfo() : 0;
		mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::preferredMetaInfoKeyList());
	}


	void setupGroup() {
		mOneFileWidget = new QWidget();

		mKeyValueWidget = new KeyValueWidget(mOneFileWidget);

		QLabel* moreLabel = new QLabel(mOneFileWidget);
		moreLabel->setText(QString("<a href='#'>%1</a>").arg(i18nc("@action show more image meta info", "More...")));
		moreLabel->setAlignment(Qt::AlignRight);

		QVBoxLayout* layout = new QVBoxLayout(mOneFileWidget);
		layout->setMargin(2);
		layout->setSpacing(2);
		layout->addWidget(mKeyValueWidget);
		layout->addWidget(moreLabel);

		mMultipleFilesLabel = new QLabel();

		mGroup = new SideBarGroup(i18nc("@title:group", "Meta Information"));
		q->setWidget(mGroup);
		mGroup->addWidget(mOneFileWidget);
		mGroup->addWidget(mMultipleFilesLabel);

		EventWatcher::install(mGroup, QEvent::Show, q, SLOT(updateSideBarContent()));

		QObject::connect(moreLabel, SIGNAL(linkActivated(const QString&)),
			q, SLOT(showMetaInfoDialog()) );
	}
};


InfoContextManagerItem::InfoContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new InfoContextManagerItemPrivate) {
	d->q = this;
	d->setupGroup();
	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );
}

InfoContextManagerItem::~InfoContextManagerItem() {
	delete d;
}


void InfoContextManagerItem::updateSideBarContent() {
	LOG("updateSideBarContent");
	if (!d->mGroup->isVisible()) {
		LOG("updateSideBarContent: not visible, not updating");
		return;
	}
	LOG("updateSideBarContent: really updating");

	KFileItemList itemList = contextManager()->selection();
	if (itemList.count() == 0) {
		// "Garbage collect" document
		d->mDocument = 0;
		d->updateMetaInfoDialog();
		return;
	}

	KFileItem item = itemList.first();
	if (itemList.count() == 1 && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
		fillOneFileGroup(item);
	} else {
		fillMultipleItemsGroup(itemList);
	}
	d->updateMetaInfoDialog();
}

void InfoContextManagerItem::fillOneFileGroup(const KFileItem& item) {
	d->mFileItem = item;
	d->mOneFileWidget->show();
	d->mMultipleFilesLabel->hide();

	d->mDocument = DocumentFactory::instance()->load(item.url());
	connect(d->mDocument.data(), SIGNAL(metaInfoUpdated()),
		SLOT(updateOneFileInfo()) );

	d->updateMetaInfoDialog();
	updateOneFileInfo();
}

void InfoContextManagerItem::fillMultipleItemsGroup(const KFileItemList& itemList) {
	// "Garbage collect" document
	d->mDocument = 0;

	int folderCount = 0, fileCount = 0;
	Q_FOREACH(const KFileItem& item, itemList) {
		if (item.isDir()) {
			folderCount++;
		} else {
			fileCount++;
		}
	}

	if (folderCount == 0) {
		d->mMultipleFilesLabel->setText(i18ncp("@label", "%1 file selected", "%1 files selected", fileCount));
	} else if (fileCount == 0) {
		d->mMultipleFilesLabel->setText(i18ncp("@label", "%1 folder selected", "%1 folders selected", folderCount));
	} else {
		d->mMultipleFilesLabel->setText(i18nc("@label. The two parameters are strings like '2 folders' and '1 file'.",
			"%1 and %2 selected", i18np("%1 folder", "%1 folders", folderCount), i18np("%1 file", "%1 files", fileCount)));
	}
	d->mOneFileWidget->hide();
	d->mMultipleFilesLabel->show();
}


void InfoContextManagerItem::updateOneFileInfo() {
	if (!d->mDocument) {
		return;
	}

	QStringList list;
	d->mKeyValueWidget->clear();
	ImageMetaInfoModel* metaInfoModel = d->mDocument->metaInfo();
	Q_FOREACH(const QString& key, GwenviewConfig::preferredMetaInfoKeyList()) {
		QString label;
		QString value;
		metaInfoModel->getInfoForKey(key, &label, &value);

		if (!label.isEmpty() && !value.isEmpty()) {
			d->mKeyValueWidget->addRow(label, value);
		}
	}

	d->mKeyValueWidget->show();
}


void InfoContextManagerItem::showMetaInfoDialog() {
	if (!d->mImageMetaInfoDialog) {
		d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mOneFileWidget);
		d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(d->mImageMetaInfoDialog, SIGNAL(preferredMetaInfoKeyListChanged(const QStringList&)),
			SLOT(slotPreferredMetaInfoKeyListChanged(const QStringList&)) );
	}
	d->mImageMetaInfoDialog->setMetaInfo(d->mDocument ? d->mDocument->metaInfo() : 0, GwenviewConfig::preferredMetaInfoKeyList());
	d->mImageMetaInfoDialog->show();
}


void InfoContextManagerItem::slotPreferredMetaInfoKeyListChanged(const QStringList& list) {
	GwenviewConfig::setPreferredMetaInfoKeyList(list);
	GwenviewConfig::self()->writeConfig();
	updateOneFileInfo();
}


} // namespace
