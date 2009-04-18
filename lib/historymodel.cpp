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

// KDE
#include <kfileplacesmodel.h>
#include <kmimetype.h>

// Local
#include <lib/gwenviewconfig.h>

namespace Gwenview {

static const int MAX_RECENT_FOLDER = 20;


struct HistoryModelPrivate {
	HistoryModel* q;

	void updateRecentFoldersModel() {
		const QStringList list = GwenviewConfig::recentFolders();

		q->clear();
		Q_FOREACH(const QString& urlString, list) {
			KUrl url(urlString);

			QStandardItem* item = new QStandardItem;
			item->setText(url.pathOrUrl());

			QString iconName = KMimeType::iconNameForUrl(url);
			item->setIcon(KIcon(iconName));

			item->setData(QVariant(url), KFilePlacesModel::UrlRole);

			q->appendRow(item);
		}
	}
};


HistoryModel::HistoryModel(QObject* parent, const QString& storageDir)
: QStandardItemModel(parent)
, d(new HistoryModelPrivate) {
	d->q = this;
	d->updateRecentFoldersModel();
}


HistoryModel::~HistoryModel() {
	delete d;
}


void HistoryModel::addUrl(const KUrl& _url) {
	KUrl url(_url);
	url.cleanPath();
	url.adjustPath(KUrl::RemoveTrailingSlash);
	QString urlString = url.url();

	QStringList list = GwenviewConfig::recentFolders();
	int index = list.indexOf(urlString);

	if (index == 0) {
		// Nothing to do, it's already the first recent folder.
		return;
	} else if (index != -1) {
		// Remove it from the list. This way it will get inserted at the
		// beginning.
		list.removeAt(index);
	}

	list.insert(0, urlString);
	while (list.size() > MAX_RECENT_FOLDER) {
		list.removeLast();
	}

	GwenviewConfig::setRecentFolders(list);
	d->updateRecentFoldersModel();
}


} // namespace
