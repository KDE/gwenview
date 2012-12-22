// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KDirModel>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KGlobal>
#include <KLocale>
#include <KMimeType>
#include <KStandardDirs>
#include <KTemporaryFile>
#include <KUrl>

// Local
#include <lib/urlutils.h>

namespace Gwenview
{

struct HistoryItem : public QStandardItem
{
    void save() const
    {
        KConfig config(mConfigPath, KConfig::SimpleConfig);
        KConfigGroup group(&config, "general");
        group.writeEntry("url", mUrl);
        group.writeEntry("dateTime", mDateTime.toString(Qt::ISODate));
        config.sync();
    }

    static HistoryItem* create(const KUrl& url, const QDateTime& dateTime, const QString& storageDir)
    {
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

    static HistoryItem* load(const QString& fileName)
    {
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
        if (UrlUtils::urlIsFastLocalFile(url)) {
            if (!QFile::exists(url.path())) {
                kDebug() << "Skipping" << url.path() << "from recent folders. It does not exist anymore";
                return 0;
            }
        }

        return new HistoryItem(url, dateTime, fileName);
    }

    KUrl url() const
    {
        return mUrl;
    }

    QDateTime dateTime() const
    {
        return mDateTime;
    }

    void setDateTime(const QDateTime& dateTime)
    {
        if (mDateTime != dateTime) {
            mDateTime = dateTime;
            save();
        }
    }

    void unlink()
    {
        QFile::remove(mConfigPath);
    }

private:
    KUrl mUrl;
    QDateTime mDateTime;
    QString mConfigPath;

    HistoryItem(const KUrl& url, const QDateTime& dateTime, const QString& configPath)
        : mUrl(url)
        , mDateTime(dateTime)
        , mConfigPath(configPath) {
        mUrl.cleanPath();
        mUrl.adjustPath(KUrl::RemoveTrailingSlash);
        setText(mUrl.pathOrUrl());

        QString iconName = KMimeType::iconNameForUrl(mUrl);
        setIcon(KIcon(iconName));

        setData(QVariant(mUrl), KFilePlacesModel::UrlRole);

        KFileItem fileItem(KFileItem::Unknown, KFileItem::Unknown, mUrl);
        setData(QVariant(fileItem), KDirModel::FileItemRole);

        QString date = KGlobal::locale()->formatDateTime(mDateTime, KLocale::FancyLongDate);
        setData(QVariant(i18n("Last visited: %1", date)), Qt::ToolTipRole);
    }

    bool operator<(const QStandardItem& other) const {
        return mDateTime > static_cast<const HistoryItem*>(&other)->mDateTime;
    }
};

struct HistoryModelPrivate
{
    HistoryModel* q;
    QString mStorageDir;
    int mMaxCount;

    QMap<KUrl, HistoryItem*> mHistoryItemForUrl;

    void load()
    {
        QDir dir(mStorageDir);
        if (!dir.exists()) {
            return;
        }
        Q_FOREACH(const QString & name, dir.entryList(QStringList() << "*rc")) {
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
                mHistoryItemForUrl.insert(item->url(), item);
                q->appendRow(item);
            }
        }
        q->sort(0);
    }

    void garbageCollect()
    {
        while (q->rowCount() > mMaxCount) {
            HistoryItem* item = static_cast<HistoryItem*>(q->takeRow(q->rowCount() - 1).at(0));
            mHistoryItemForUrl.remove(item->url());
            item->unlink();
            delete item;
        }
    }
};

HistoryModel::HistoryModel(QObject* parent, const QString& storageDir, int maxCount)
: QStandardItemModel(parent)
, d(new HistoryModelPrivate)
{
    d->q = this;
    d->mStorageDir = storageDir;
    d->mMaxCount = maxCount;
    d->load();
}

HistoryModel::~HistoryModel()
{
    delete d;
}

void HistoryModel::addUrl(const KUrl& url, const QDateTime& _dateTime)
{
    QDateTime dateTime = _dateTime.isValid() ? _dateTime : QDateTime::currentDateTime();
    HistoryItem* historyItem = d->mHistoryItemForUrl.value(url);
    if (historyItem) {
        historyItem->setDateTime(dateTime);
        sort(0);
    } else {
        historyItem = HistoryItem::create(url, dateTime, d->mStorageDir);
        if (!historyItem) {
            kError() << "Could not save history for url" << url;
            return;
        }
        appendRow(historyItem);
        sort(0);
        d->garbageCollect();
    }
}

bool HistoryModel::removeRows(int start, int count, const QModelIndex& parent)
{
    Q_ASSERT(!parent.isValid());
    for (int row = start + count - 1; row >= start ; --row) {
        HistoryItem* historyItem = static_cast<HistoryItem*>(item(row, 0));
        Q_ASSERT(historyItem);
        d->mHistoryItemForUrl.remove(historyItem->url());
        historyItem->unlink();
    }
    return QStandardItemModel::removeRows(start, count, parent);
}

} // namespace
