// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "historymodel.moc"

// Qt
#include <QDateTime>
#include <QDir>
#include <QFile>

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kfileplacesmodel.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kurl.h>

// Local

namespace Gwenview {

static const int MAX_HISTORY_SIZE = 20;


struct HistoryItem {
	void save() const {
		KConfig config(mConfigPath, KConfig::SimpleConfig);
		KConfigGroup group(&config, "general");
		group.writeEntry("url", mUrl);
		group.writeEntry("dateTime", mDateTime.toString(Qt::ISODate));
		config.sync();
	}

	static HistoryItem* create(const KUrl& url, const QDateTime& dateTime, const QString& storageDir) {
		if (!KStandardDirs::makeDir(storageDir, 0600)) {
			kError() << "Could not create history dir" << storageDir;
			return 0;
		}
		KTemporaryFile file;
		file.setAutoRemove(false);
		file.setPrefix(storageDir);
		file.setSuffix("rc");
		if (!file.open()) {
			kError() << "Could not create history file";
			return 0;
		}

		HistoryItem* item = new HistoryItem(url, dateTime, file.fileName());
		item->save();
		return item;
	}

	static HistoryItem* load(const QString& fileName) {
		KConfig config(fileName, KConfig::SimpleConfig);
		KConfigGroup group(&config, "general");

		KUrl url(group.readEntry("url"));
		if (!url.isValid()) {
			kError() << "Invalid url" << url;
			return 0;
		}
		QDateTime dateTime = QDateTime::fromString(group.readEntry("dateTime"), Qt::ISODate);
		if (!dateTime.isValid()) {
			kError() << "Invalid dateTime" << dateTime;
			return 0;
		}

		return new HistoryItem(url, dateTime, fileName);
	}

	KUrl url() const {
		return mUrl;
	}

	QDateTime dateTime() const {
		return mDateTime;
	}

	void setDateTime(const QDateTime& dateTime) {
		if (mDateTime != dateTime) {
			mDateTime = dateTime;
			save();
		}
	}

	void unlink() {
		QFile::remove(mConfigPath);
	}

private:
	KUrl mUrl;
	QDateTime mDateTime;
	QString mConfigPath;

	HistoryItem(const KUrl& url, const QDateTime& dateTime, const QString& configPath)
	: mUrl(url)
	, mDateTime(dateTime)
	, mConfigPath(configPath)
	{
		mUrl.cleanPath();
		mUrl.adjustPath(KUrl::RemoveTrailingSlash);
	}
};


bool historyItemLessThan(const HistoryItem* item1, const HistoryItem* item2) {
	return item1->dateTime() > item2->dateTime();
}


struct HistoryModelPrivate {
	HistoryModel* q;
	QString mStorageDir;

	QList<HistoryItem*> mHistoryItemList;
	QMap<KUrl, HistoryItem*> mHistoryItemForUrl;

	void updateModelItems() {
		// FIXME: optimize
		q->clear();
		Q_FOREACH(const HistoryItem* historyItem, mHistoryItemList) {
			KUrl url(historyItem->url());

			QStandardItem* item = new QStandardItem;
			item->setText(url.pathOrUrl());

			QString iconName = KMimeType::iconNameForUrl(url);
			item->setIcon(KIcon(iconName));

			item->setData(QVariant(url), KFilePlacesModel::UrlRole);

			QString date = KGlobal::locale()->formatDateTime(historyItem->dateTime(), KLocale::FancyLongDate);
			item->setData(QVariant(i18n("Last visited: %1", date)), Qt::ToolTipRole);

			q->appendRow(item);
		}
	}

	void load() {
		QDir dir(mStorageDir);
		if (!dir.exists()) {
			return;
		}
		Q_FOREACH(const QString& name, dir.entryList(QStringList() << "*rc")) {
			HistoryItem* item = HistoryItem::load(dir.filePath(name));
			if (!item) {
				continue;
			}
			HistoryItem* existingItem = mHistoryItemForUrl.value(item->url());
			if (existingItem) {
				// We already know this url(!) update existing item dateTime
				// and get rid of duplicate
				if (existingItem->dateTime() < item->dateTime()) {
					existingItem->setDateTime(item->dateTime());
				}
				item->unlink();
				delete item;
			} else {
				mHistoryItemList << item;
				mHistoryItemForUrl.insert(item->url(), item);
			}
		}
		sortList();
		updateModelItems();
	}

	void sortList() {
		qSort(mHistoryItemList.begin(), mHistoryItemList.end(), historyItemLessThan);
	}

	void garbageCollect() {
		while (mHistoryItemList.count() > MAX_HISTORY_SIZE) {
			HistoryItem* item = mHistoryItemList.takeLast();
			mHistoryItemForUrl.remove(item->url());
			item->unlink();
			delete item;
		}
	}
};


HistoryModel::HistoryModel(QObject* parent, const QString& storageDir)
: QStandardItemModel(parent)
, d(new HistoryModelPrivate) {
	d->q = this;
	d->mStorageDir = storageDir;
	d->load();
}


HistoryModel::~HistoryModel() {
	qDeleteAll(d->mHistoryItemList);
	delete d;
}


void HistoryModel::addUrl(const KUrl& url) {
	HistoryItem* historyItem = d->mHistoryItemForUrl.value(url);
	if (historyItem) {
		historyItem->setDateTime(QDateTime::currentDateTime());
		d->sortList();
	} else {
		historyItem = HistoryItem::create(url, QDateTime::currentDateTime(), d->mStorageDir);
		if (!historyItem) {
			kError() << "Could not save history for url" << url;
			return;
		}
		d->mHistoryItemList << historyItem;
		d->sortList();
		d->garbageCollect();
	}
	d->updateModelItems();
}


bool HistoryModel::removeRows(int start, int count, const QModelIndex& parent) {
	Q_ASSERT(!parent.isValid());
	for (int row=start + count - 1; row >= start ; --row) {
		HistoryItem* historyItem = d->mHistoryItemList.takeAt(row);
		Q_ASSERT(historyItem);
		d->mHistoryItemForUrl.remove(historyItem->url());
		historyItem->unlink();
		delete historyItem;
	}
	return QStandardItemModel::removeRows(start, count, parent);
}


} // namespace
