// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "thumbnailviewhelper.moc"

// Qt
#include <QCursor>
#include <QIcon>
#include <QPainter>
#include <QPointer>

// KDE
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kiconloader.h>
#include <kio/previewjob.h>
#include <klocale.h>
#include <kmenu.h>
#include <kdebug.h>
#include <kpropertiesdialog.h>

// Local
#include "fileopscontextmanageritem.h"
#include <lib/document/documentfactory.h>
#include <lib/mimetypeutils.h>
#include <lib/thumbnailloadjob.h>

namespace Gwenview {

const int THUMBNAIL_SIZE = 256;


struct ThumbnailViewHelperPrivate {
	FileOpsContextManagerItem* mFileOpsContextManagerItem;
	QPointer<ThumbnailLoadJob> mThumbnailLoadJob;
};


ThumbnailViewHelper::ThumbnailViewHelper(QObject* parent)
: AbstractThumbnailViewHelper(parent)
, d(new ThumbnailViewHelperPrivate) {
}


ThumbnailViewHelper::~ThumbnailViewHelper() {
	delete d;
}


void ThumbnailViewHelper::generateThumbnailsForItems(const KFileItemList& list) {
	KFileItemList filteredList;
	DocumentFactory* factory = DocumentFactory::instance();
	Q_FOREACH(KFileItem item, list) {
		MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
		if (kind == MimeTypeUtils::KIND_DIR || kind == MimeTypeUtils::KIND_ARCHIVE) {
			continue;
		}

		if (factory->hasUrl(item.url())) {
			Document::Ptr doc = factory->load(item.url());
			if (doc->isLoaded() && doc->isModified()) {
				QImage image = doc->image();
				if (image.width() > THUMBNAIL_SIZE || image.height() > THUMBNAIL_SIZE) {
					image = image.scaled(THUMBNAIL_SIZE, THUMBNAIL_SIZE, Qt::KeepAspectRatio);
				}
				QPainter painter(&image);
				QPixmap pix = SmallIcon("document-save");
				painter.drawPixmap(
					image.width() - pix.width(),
					image.height() - pix.height(),
					pix);
				painter.end();
				thumbnailLoaded(item, QPixmap::fromImage(image));
				continue;
			}
		}

		filteredList << item;
	}
	if (filteredList.size() > 0) {
		if (!d->mThumbnailLoadJob) {
			d->mThumbnailLoadJob = new ThumbnailLoadJob(filteredList, THUMBNAIL_SIZE);
			connect(d->mThumbnailLoadJob, SIGNAL(thumbnailLoaded(const KFileItem&, const QPixmap&, const QSize&)),
				SIGNAL(thumbnailLoaded(const KFileItem&, const QPixmap&)));
			d->mThumbnailLoadJob->start();
		} else {
			Q_FOREACH(KFileItem item, filteredList) {
				d->mThumbnailLoadJob->appendItem(item);
			}
		}
	}
}


void ThumbnailViewHelper::abortThumbnailGenerationForItems(const KFileItemList& list) {
	if (!d->mThumbnailLoadJob) {
		return;
	}
	Q_FOREACH(KFileItem item, list) {
		kDebug() << "aborting" << item.url();
		d->mThumbnailLoadJob->itemRemoved(item);
	}
}


void ThumbnailViewHelper::setFileOpsContextManagerItem(FileOpsContextManagerItem* item) {
	d->mFileOpsContextManagerItem = item;
}


inline void addIfEnabled(KMenu& popup, QAction* action) {
	if (action->isEnabled()) {
		popup.addAction(action);
	}
}


void ThumbnailViewHelper::showContextMenu(QWidget* parent) {
	KMenu popup(parent);
	addIfEnabled(popup, d->mFileOpsContextManagerItem->copyToAction());
	addIfEnabled(popup, d->mFileOpsContextManagerItem->moveToAction());
	addIfEnabled(popup, d->mFileOpsContextManagerItem->linkToAction());
	popup.addSeparator();
	addIfEnabled(popup, d->mFileOpsContextManagerItem->trashAction());
	addIfEnabled(popup, d->mFileOpsContextManagerItem->delAction());
	popup.addSeparator();
	addIfEnabled(popup, d->mFileOpsContextManagerItem->showPropertiesAction());
	popup.exec(QCursor::pos());
}


} // namespace
