// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*  Gwenview - A simple image viewer for KDE
    Copyright 2000-2004 Aurélien Gâteau
    This class is based on the ImagePreviewJob class from Konqueror.
    Original copyright follows.
*/
/*  This file is part of the KDE project
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

// KDE includes
#include <kio/job.h>

// Our includes
#include "tsthread/tsthread.h"
#include "tsthread/tswaitcondition.h"
#include "thumbnailsize.h"

class KFileItem;

typedef QPtrList<KFileItem> KFileItemList;

class ThumbnailThread : public TSThread {
Q_OBJECT
public:
	void load(
		const QString& originalURI,
		time_t originalTime,
		int originalSize,
		const QString& originalMimeType,
		const QString& pixPath,
		const QString& thumbnailPath);
	QImage popThumbnail();
protected:
	virtual void run();
signals:
	void done();
private:
	bool isJPEG(const QString& name);
	bool loadJPEG(const QString &pixPath, QImage&, int& width, int& height);
	void loadThumbnail();
	QImage mImage;
	QString mPixPath;
	QString mThumbnailPath;
	QString mOriginalURI;
	time_t mOriginalTime;
	int mOriginalSize;
	QString mOriginalMimeType;
	QMutex mMutex;
	TSWaitCondition mCond;
};

/**
 * A job that determines the thumbnails for the images in the current directory
 */
class ThumbnailLoadJob : public KIO::Job {
Q_OBJECT
public:
	/**
	 * Create a job for determining the pixmaps of the images in the @p itemList
	 */
	ThumbnailLoadJob( const KFileItemList* itemList,ThumbnailSize size);
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
	 * Set item to be the next processed job. Returns false
	 * if there's no such item in mItems
	 */	
	bool setNextItem(const KFileItem* item);

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
	 * Returns the thumbnail base dir
	 */
	static QString thumbnailBaseDir();


	/**
	 * Delete the thumbnail for the @p url
	 */
	static void deleteImageThumbnail(const KURL& url);
	
signals:
	/**
	 * Emitted when the thumbnail for the @p item has been loaded
	 */
	void thumbnailLoaded(const KFileItem* item, const QPixmap&, const QSize&);

private slots:
	void slotResult( KIO::Job *job );
	void checkThumbnail();
	void thumbnailReady();

private:
	enum { STATE_STATORIG, STATE_DOWNLOADORIG, STATE_DELETETEMP, STATE_CREATETHUMB, STATE_NEXTTHUMB } mState;

	// Our todo list :)
	KFileItemList mItems;

	// The current item
	const KFileItem *mCurrentItem;

	// The next item to be processed
	const KFileItem *mNextItem;

	// The URL of the current item (always equivalent to m_items.first()->item()->url())
	KURL mCurrentURL;

	// The URI of the original image (might be different from mCurrentURL.url())
	QString mOriginalURI;
	
	// The modification time of the original image
	time_t mOriginalTime;

	// The thumbnail path
	QString mThumbnailPath;

	// The URL of the temporary file for remote urls
	KURL mTempURL;

	// Thumbnail size
	ThumbnailSize mThumbnailSize;

	QPixmap mBrokenPixmap;

	bool mSuspended;
        
	ThumbnailThread mThumbnailThread;

	void determineNextIcon();
	void startCreatingThumbnail(const QString& path);
	
	void emitThumbnailLoaded(const QImage& img);
	void emitThumbnailLoadingFailed();
};

#endif
