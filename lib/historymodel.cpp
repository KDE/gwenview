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

// KDE
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kfileplacesmodel.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <kurl.h>

// Local

namespace Gwenview {

//static const int MAX_RECENT_FOLDER = 20;


struct HistoryItem {
	HistoryItem() {}
	HistoryItem(const KUrl& _url, const QDateTime& _dateTime)
	: mUrl(_url)
	, mDateTime(_dateTime)
	{
		mUrl.cleanPath();
		mUrl.adjustPath(KUrl::RemoveTrailingSlash);
	}

	bool save(const QString& storageDir) const {
		if (!KStandardDirs::makeDir(storageDir, 0600)) {
			kError() << "Could not create history dir" << storageDir;
			return false;
		}
		KTemporaryFile file;
		file.setAutoRemove(false);
		file.setPrefix(storageDir);
		file.setSuffix("rc");
		if (!file.open()) {
			kError() << "Could not create history file";
			return false;
		}

		KConfig config(file.fileName(), KConfig::SimpleConfig);
		KConfigGroup group(&config, "general");
		group.writeEntry("url", mUrl);
		group.writeEntry("dateTime", mDateTime.toString(Qt::ISODate));
		config.sync();
		return true;
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

		return new HistoryItem(url, dateTime);
	}

	KUrl url() const {
		return mUrl;
	}

	QDateTime dateTime() const {
		return mDateTime;
	}

	void setDateTime(const QDateTime& dateTime) {
		mDateTime = dateTime;
	}

private:
	KUrl mUrl;
	QDateTime mDateTime;
};


bool historyItemLessThan(const HistoryItem* item1, const HistoryItem* item2) {
	return item1->dateTime() > item2->dateTime();
}


struct HistoryModelPrivate {
	HistoryModel* q;
	QString mStorageDir;

	QList<HistoryItem*> mHistoryItemList;

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
			if (item) {
				mHistoryItemList << item;
			}
		}
		sortList();
		updateModelItems();
	}

	void sortList() {
		qSort(mHistoryItemList.begin(), mHistoryItemList.end(), historyItemLessThan);
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
	HistoryItem* historyItem = new HistoryItem(url, QDateTime::currentDateTime());
	if (!historyItem->save(d->mStorageDir)) {
		kError() << "Could not save history for url" << url;
		delete historyItem;
		return;
	}
	d->mHistoryItemList << historyItem;
	// FIXME: garbageCollect
	d->sortList();
	d->updateModelItems();
}


} // namespace
