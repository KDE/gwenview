// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*	Gwenview - A simple image viewer for KDE
	Copyright 2000-2004 Aurélien Gâteau <aurelien.gateau@free.fr>
	This class is based on the ImagePreviewJob class from Konqueror.
*/
/*	This file is part of the KDE project
	Copyright (C) 2000 David Faure <faure@kde.org>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef THUMBNAILLOADJOB_H
#define THUMBNAILLOADJOB_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QImage>
#include <QPixmap>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

// KDE
#include <kio/job.h>
#include <kfileitem.h>

// Local

namespace Gwenview {
class ThumbnailThread : public QThread {
Q_OBJECT
public:
	ThumbnailThread();

	void load(
		const QString& originalUri,
		time_t originalTime,
		int originalSize,
		const QString& originalMimeType,
		const QString& pixPath,
		const QString& thumbnailPath,
		int size);

	void cancel();

protected:
	virtual void run();

Q_SIGNALS:
	void done( const QImage&, const QSize&);

private:
	bool testCancel();
	bool isJpeg();
	bool loadJpeg();
	void loadThumbnail();
	QImage mImage;
	QString mPixPath;
	QString mThumbnailPath;
	QString mOriginalUri;
	time_t mOriginalTime;
	int mOriginalSize;
	QString mOriginalMimeType;
	int mOriginalWidth;
	int mOriginalHeight;
	QMutex mMutex;
	QWaitCondition mCond;
	int mThumbnailSize;
	bool mCancel;
};

/**
 * A job that determines the thumbnails for the images in the current directory
 */
class GWENVIEWLIB_EXPORT ThumbnailLoadJob : public KIO::Job {
Q_OBJECT
public:
	/**
	 * Create a job for determining the pixmaps of the images in the @p itemList
	 */
	ThumbnailLoadJob(const KFileItemList& itemList, int size);
	virtual ~ThumbnailLoadJob();

	/**
	 * Call this to get started
	 */
	void start();

	/**
	 * To be called whenever items are removed from the view
	 */
	void removeItems(const KFileItemList& itemList);

	/**
	 * Returns the list of items waiting for a thumbnail
	 */
	const KFileItemList& pendingItems() const;

	/**
	 * Add an item to a running job
	 */
	void appendItem(const KFileItem& item);

	/**
	 * Returns the thumbnail base dir, independent of the thumbnail size
	 */
	static QString thumbnailBaseDir();

	/**
	 * Sets the thumbnail base dir, useful for unit-testing
	 */
	static void setThumbnailBaseDir(const QString&);

	/**
	 * Returns the thumbnail base dir, for the @p size
	 */
	static QString thumbnailBaseDir(int size);


	/**
	 * Delete the thumbnail for the @p url
	 */
	static void deleteImageThumbnail(const KUrl& url);

Q_SIGNALS:
	/**
	 * Emitted when the thumbnail for the @p item has been loaded
	 */
	void thumbnailLoaded(const KFileItem& item, const QPixmap&, const QSize&);

protected:
	virtual void slotResult( KJob *job );

private Q_SLOTS:
	void slotGotPreview(const KFileItem&, const QPixmap&);
	void checkThumbnail();
	void thumbnailReady(const QImage& im, const QSize&);
	void emitThumbnailLoadingFailed();

private:
	enum { STATE_STATORIG, STATE_DOWNLOADORIG, STATE_PREVIEWJOB, STATE_NEXTTHUMB } mState;

	KFileItemList mItems;
	KFileItem mCurrentItem;

	// The Url of the current item (always equivalent to m_items.first()->item()->url())
	KUrl mCurrentUrl;

	// The Uri of the original image (might be different from mCurrentUrl.url())
	QString mOriginalUri;

	// The modification time of the original image
	time_t mOriginalTime;

	// The thumbnail path
	QString mThumbnailPath;

	// The temporary path for remote urls
	QString mTempPath;

	// Thumbnail size
	int mThumbnailSize;

	QPixmap mBrokenPixmap;

	ThumbnailThread mThumbnailThread;

	void determineNextIcon();
	void startCreatingThumbnail(const QString& path);

	void emitThumbnailLoaded(const QImage& img, const QSize& size);
};

} // namespace
#endif

