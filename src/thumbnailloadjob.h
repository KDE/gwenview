/*  Gwenview - A simple image viewer for KDE
    Copyright (c) 2000-2003 Aurélien Gâteau
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef THUMBNAILLOADJOB_H
#define THUMBNAILLOADJOB_H

// KDE includes
#include <kio/job.h>

// Our includes
#include "thumbnailsize.h"

class KFileItem;
class QPixmap;

typedef QList<KFileItem> KFileItemList;


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
	 * Returns the thumbnail dir
	 */
	static const QString& thumbnailDir();

signals:
	/**
	 * Emitted when the thumbnail for the @p item has been loaded
	 */
	void thumbnailLoaded(const KFileItem* item,const QPixmap&);

private slots:
	void slotResult( KIO::Job *job );

private:
	enum { STATE_STATORIG, STATE_STATTHUMB, STATE_DOWNLOADORIG, STATE_DELETETEMP } mState;

	// Our todo list :)
	KFileItemList mItems;

	// The current item
	KFileItem *mCurrentItem;

	// The URL of the current item (always equivalent to m_items.first()->item()->url())
	KURL mCurrentURL;

	// The modification time of that URL
	time_t mOriginalTime;

	// The URL where we find (or create) the thumbnail for the current URL
	KURL mThumbURL;

	// The URL of the temporary file for remote urls
	KURL mTempURL;

	// Thumbnail cache dir
	QString mCacheDir;

	// Thumbnail size
	ThumbnailSize mThumbnailSize;

	void determineNextIcon();
	bool statResultThumbnail( KIO::StatJob * );
	void createThumbnail(const QString& path);
	
	bool isJPEG(const QString& name);
	bool loadJPEG( const QString &pixPath, QPixmap &pix);
	bool loadThumbnail(const QString& pixPath, QPixmap &pix);
	void emitThumbnailLoaded(const QPixmap &pix);
};

#endif
