// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "thumbnailviewhelper.moc"

// Qt
#include <QCursor>
#include <QPointer>

// KDE
#include <kdebug.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kiconloader.h>
#include <kio/copyjob.h>
#include <kio/previewjob.h>
#include <klocale.h>
#include <kmenu.h>
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
	KUrl mCurrentDirUrl;
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
	Q_FOREACH(const KFileItem& item, list) {
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
			Q_FOREACH(const KFileItem& item, filteredList) {
				d->mThumbnailLoadJob->appendItem(item);
			}
		}
	}
}


void ThumbnailViewHelper::abortThumbnailGenerationForItems(const KFileItemList& list) {
	if (d->mThumbnailLoadJob) {
		d->mThumbnailLoadJob->removeItems(list);
	}
}


void ThumbnailViewHelper::setCurrentDirUrl(const KUrl& url) {
	d->mCurrentDirUrl = url;
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
	addIfEnabled(popup, d->mFileOpsContextManagerItem->createFolderAction());
	popup.addSeparator();
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


bool ThumbnailViewHelper::isDocumentModified(const KUrl& url) {
	DocumentFactory* factory = DocumentFactory::instance();

	if (factory->hasUrl(url)) {
		Document::Ptr doc = factory->load(url);
		return doc->isLoaded() && doc->isModified();
	} else {
		return false;
	}
}


void ThumbnailViewHelper::showMenuForUrlDroppedOnViewport(QWidget* parent, const KUrl::List& lst) {
	showMenuForUrlDroppedOnDir(parent, lst, d->mCurrentDirUrl);
}


void ThumbnailViewHelper::showMenuForUrlDroppedOnDir(QWidget* parent, const KUrl::List& urlList, const KUrl& destUrl) {
	KMenu menu(parent);
	QAction* moveAction = menu.addAction(
		KIcon("goto-page"),
		i18n("Move Here"));
	QAction* copyAction = menu.addAction(
		KIcon("edit-copy"),
		i18n("Copy Here"));
	QAction* linkAction = menu.addAction(
		KIcon("www"),
		i18n("Link Here"));
	menu.addSeparator();
	menu.addAction(
		KIcon("process-stop"),
		i18n("Cancel"));

	QAction* action = menu.exec(QCursor::pos());

	if (action == moveAction) {
		KIO::move(urlList, destUrl);
	} else if (action == copyAction) {
		KIO::copy(urlList, destUrl);
	} else if (action == linkAction) {
		KIO::link(urlList, destUrl);
	}
}


} // namespace
