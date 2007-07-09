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
#include "sorteddirmodel.moc"

// Qt
#include <QIcon>
#include <QPainter>

// KDE
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kiconloader.h>
#include <kio/previewjob.h>

// Local
#include "documentfactory.h"

namespace Gwenview {

struct SortedDirModelPrivate {
	KDirModel* mSourceModel;
};


SortedDirModel::SortedDirModel(QObject* parent)
: QSortFilterProxyModel(parent)
, d(new SortedDirModelPrivate)
{
	d->mSourceModel = new KDirModel(this);
	setSourceModel(d->mSourceModel);
	setDynamicSortFilter(true);
	sort(KDirModel::Name);
}


SortedDirModel::~SortedDirModel() {
	delete d;
}


KDirLister* SortedDirModel::dirLister() {
	return d->mSourceModel->dirLister();
}


KFileItem* SortedDirModel::itemForIndex(const QModelIndex& index) const {
	if (!index.isValid()) {
		return 0;
	}

	QModelIndex sourceIndex = mapToSource(index);
	return d->mSourceModel->itemForIndex(sourceIndex);
}


QModelIndex SortedDirModel::indexForItem(const KFileItem& item) const {
	if (item.isNull()) {
		return QModelIndex();
	}

	QModelIndex sourceIndex = d->mSourceModel->indexForItem(item);
	return mapFromSource(sourceIndex);
}


QModelIndex SortedDirModel::indexForUrl(const KUrl& url) const {
	QModelIndex sourceIndex = d->mSourceModel->indexForUrl(url);
	return mapFromSource(sourceIndex);
}


static bool kFileItemLessThan(const KFileItem* leftItem, const KFileItem* rightItem) {
	bool leftIsDir = leftItem->isDir();
	bool rightIsDir = rightItem->isDir();
	if (leftIsDir && !rightIsDir) {
		return true;
	}
	if (!leftIsDir && rightIsDir) {
		return false;
	}
	return leftItem->name().toLower() < rightItem->name().toLower();
}


bool SortedDirModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	KFileItem* leftItem = d->mSourceModel->itemForIndex(left);
	KFileItem* rightItem = d->mSourceModel->itemForIndex(right);
	Q_ASSERT(leftItem);
	Q_ASSERT(rightItem);

	return kFileItemLessThan(leftItem, rightItem);
}


void SortedDirModel::generateThumbnailsForItems(const QList<KFileItem>& list, int size) {
	QList<KFileItem> filteredList;
	DocumentFactory* factory = DocumentFactory::instance();
	Q_FOREACH(KFileItem item, list) {
		if (factory->hasUrl(item.url())) {
			Document::Ptr doc = factory->load(item.url());
			doc->waitUntilLoaded();
			QImage image = doc->image().scaled(size, size, Qt::KeepAspectRatio);
			if (doc->isModified()) {
				QPainter painter(&image);
				QPixmap pix = SmallIcon("document-save");
				painter.drawPixmap(
					image.width() - pix.width(),
					image.height() - pix.height(),
					pix);
			}
			setItemPreview(item, QPixmap::fromImage(image));
		} else {
			filteredList << item;
		}
	}
	if (filteredList.size() > 0) {
		KIO::PreviewJob* job = KIO::filePreview(filteredList, size);
		connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
			SLOT(setItemPreview(const KFileItem&, const QPixmap&)));
	}
}


void SortedDirModel::setItemPreview(const KFileItem& item, const QPixmap& pixmap) {
	Q_ASSERT(!item.isNull());
	QModelIndex index = indexForItem(item);
	if (!index.isValid()) {
		kWarning() << "setItemPreview: invalid index\n";
		return;
	}
	setData(index, QIcon(pixmap), Qt::DecorationRole);
}

} //namespace
