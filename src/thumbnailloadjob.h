// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*	Gwenview - A simple image viewer for KDE
	Copyright 2000-2004 Aurélien Gâteau
	This class is based on the ImagePreviewJob class from Konqueror.
	Original copyright follows.
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
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef THUMBNAILLOADJOB_H
#define THUMBNAILLOADJOB_H

// Qt includes
#include <qpixmap.h>
#include <qvaluelist.h>
#include <qvaluevector.h>

// KDE includes
#include <kio/job.h>

// Our includes
#include "tsthread/tsthread.h"
#include "tsthread/tswaitcondition.h"

#include "libgwenview_export.h"

class KFileItem;

typedef QPtrList<KFileItem> KFileItemList;

class LIBGWENVIEW_EXPORT ThumbnailThread : public TSThread {
Q_OBJECT
public:
	void load(
		const QString& originalURI,
		time_t originalTime,
		int originalSize,
		const QString& originalMimeType,
		const QString& pixPath,
		const QString& thumbnailPath,
		int size,
		bool storeThumbnail);
protected:
	virtual void run();
signals:
	void done( const QImage&, const QSize&);
private:
	bool isJPEG();
	bool loadJPEG();
	void loadThumbnail();
	QImage mImage;
	QString mPixPath;
	QString mThumbnailPath;
	QString mOriginalURI;
	time_t mOriginalTime;
	int mOriginalSize;
	QString mOriginalMimeType;
	int mOriginalWidth;
	int mOriginalHeight;
	QMutex mMutex;
	TSWaitCondition mCond;
	int mThumbnailSize;
	bool mStoreThumbnailsInCache;
};

/**
 * A job that determines the thumbnails for the images in the current directory
 */
class LIBGWENVIEW_EXPORT ThumbnailLoadJob : public KIO::Job {
Q_OBJECT
public:
	/**
	 * Create a job for determining the pixmaps of the images in the @p itemList
	 */
	ThumbnailLoadJob(const QValueVector<const KFileItem*>* itemList, int size);
	virtual ~ThumbnailLoadJob();

	/**
	 * Call this to get started
	 */
	void start();

	/**
	 * To be called whenever an item is removed from the view
	 */
	void itemRemoved(const KFileItem* item);


	/**
	 * Add an item to a running job
	 */
	void appendItem(const KFileItem* item);

	
	/**
	 * Sets items in range first..last to be generated first, starting with current.
	 */	
	void setPriorityItems(const KFileItem* current, const KFileItem* first, const KFileItem* last);

	/**
	 * Temporarily suspends loading. Used if there's a more
	 * important action going on (loading an image etc.).
	 */
	void suspend();

	/**
	 * Resumes loading if suspended.
	 */
	void resume();

	/**
	 * Returns the thumbnail base dir, independent of the thumbnail size
	 */
	static QString thumbnailBaseDir();
	
	/**
	 * Returns the thumbnail base dir, for the @p size
	 */
	static QString thumbnailBaseDir(int size);


	/**
	 * Delete the thumbnail for the @p url
	 */
	static void deleteImageThumbnail(const KURL& url);

	static void readConfig(KConfig*,const QString& group);
	static void writeConfig(KConfig*,const QString& group);

	static bool storeThumbnailsInCache();
	static void setStoreThumbnailsInCache(bool);

signals:
	/**
	 * Emitted when the thumbnail for the @p item has been loaded
	 */
	void thumbnailLoaded(const KFileItem* item, const QPixmap&, const QSize&);

private slots:
	void slotResult( KIO::Job *job );
	void checkThumbnail();
	void thumbnailReady(const QImage& im, const QSize&);

private:
	enum { STATE_STATORIG, STATE_DOWNLOADORIG, STATE_NEXTTHUMB } mState;

	QValueList<const KFileItem*> mItems;
	QValueVector<const KFileItem* > mAllItems;
	QValueVector< bool > mProcessedState;
	const KFileItem *mCurrentItem;
	int thumbnailIndex( const KFileItem* item ) const;
	void updateItemsOrder();

	// indexes of the current, fist and last visible thumbnails
	int mCurrentVisibleIndex, mFirstVisibleIndex, mLastVisibleIndex;

	// The URL of the current item (always equivalent to m_items.first()->item()->url())
	KURL mCurrentURL;

	// The URI of the original image (might be different from mCurrentURL.url())
	QString mOriginalURI;
	
	// The modification time of the original image
	time_t mOriginalTime;

	// The thumbnail path
	QString mThumbnailPath;

	// The temporary path for remote urls
	QString mTempPath;

	// Thumbnail size
	int mThumbnailSize;

	QPixmap mBrokenPixmap;

	bool mSuspended;

	ThumbnailThread mThumbnailThread;

	void determineNextIcon();
	void startCreatingThumbnail(const QString& path);
	
	void emitThumbnailLoaded(const QImage& img, QSize size);
	void emitThumbnailLoadingFailed();

	void updateItemsOrderHelper( int forward, int backward, int first, int last );
};

inline
int ThumbnailLoadJob::thumbnailIndex( const KFileItem* item ) const {
	QValueVector<const KFileItem* >::ConstIterator pos = qFind( mAllItems.begin(), mAllItems.end(), item );
	if( pos != mAllItems.end()) return pos - mAllItems.begin();
	return -1;
}

#endif
