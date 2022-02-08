// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>
Copyright 2014 Tomas Mecir <mecirt@gmail.com>

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
#include "recentfilesmodel.h"

// Qt
#include <QDir>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QUrl>

// KF
#include <KDirModel>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KFormat>

// Local
#include "gwenview_lib_debug.h"
#include <lib/urlutils.h>

namespace Gwenview
{
struct RecentFilesItem : public QStandardItem {
    QUrl url() const
    {
        return mUrl;
    }

    RecentFilesItem(const QUrl &url)
        : mUrl(url)
    {
        QString text(mUrl.toDisplayString(QUrl::PreferLocalFile));
#ifdef Q_OS_UNIX
        text.replace(QRegularExpression(QStringLiteral("^") + QDir::homePath()), QStringLiteral("~"));
#endif
        setText(text);

        QMimeDatabase db;
        const QString iconName = db.mimeTypeForUrl(mUrl).iconName();
        setIcon(QIcon::fromTheme(iconName));

        setData(mUrl, KFilePlacesModel::UrlRole);

        KFileItem fileItem(mUrl);
        setData(QVariant(fileItem), KDirModel::FileItemRole);
    }

private:
    QUrl mUrl;
};

struct RecentFilesModelPrivate {
    RecentFilesModel *q = nullptr;

    QMap<QUrl, RecentFilesItem *> mRecentFilesItemForUrl;
};

RecentFilesModel::RecentFilesModel(QObject *parent)
    : QStandardItemModel(parent)
    , d(new RecentFilesModelPrivate)
{
    d->q = this;
}

RecentFilesModel::~RecentFilesModel()
{
    delete d;
}

void RecentFilesModel::addUrl(const QUrl &url)
{
    RecentFilesItem *historyItem = d->mRecentFilesItemForUrl.value(url);
    if (!historyItem) {
        historyItem = new RecentFilesItem(url);
        if (!historyItem)
            return;
        d->mRecentFilesItemForUrl.insert(url, historyItem);
        appendRow(historyItem);
    }
    sort(0);
}

bool RecentFilesModel::removeRows(int start, int count, const QModelIndex &parent)
{
    Q_ASSERT(!parent.isValid());
    for (int row = start + count - 1; row >= start; --row) {
        auto *historyItem = static_cast<RecentFilesItem *>(item(row, 0));
        Q_ASSERT(historyItem);
        d->mRecentFilesItemForUrl.remove(historyItem->url());
    }
    return QStandardItemModel::removeRows(start, count, parent);
}

} // namespace
