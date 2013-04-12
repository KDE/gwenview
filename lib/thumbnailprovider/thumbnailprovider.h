// vim: set tabstop=4 shiftwidth=4 expandtab
/*  Gwenview - A simple image viewer for KDE
    Copyright 2000-2004 Aurélien Gâteau <agateau@kde.org>
    This class is based on the ImagePreviewJob class from Konqueror.
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef THUMBNAILPROVIDER_H
#define THUMBNAILPROVIDER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QImage>
#include <QPixmap>
#include <QPointer>

// KDE
#include <KIO/Job>
#include <KFileItem>

// Local
#include <lib/thumbnailgroup.h>

namespace Gwenview
{

class ThumbnailGenerator;
class ThumbnailWriter;

/**
 * A job that determines the thumbnails for the images in the current directory
 */
class GWENVIEWLIB_EXPORT ThumbnailProvider : public KIO::Job
{
    Q_OBJECT
public:
    ThumbnailProvider();
    virtual ~ThumbnailProvider();

    void stop();

    /**
     * To be called whenever items are removed from the view
     */
    void removeItems(const KFileItemList& itemList);

    /**
     * Remove all pending items
     */
    void removePendingItems();

    /**
     * Returns the list of items waiting for a thumbnail
     */
    const KFileItemList& pendingItems() const;

    /**
     * Add items to the job
     */
    void appendItems(const KFileItemList& items);

    /**
     * Defines size of thumbnails to generate
     */
    void setThumbnailGroup(ThumbnailGroup::Enum);

    bool isRunning() const;

    /**
     * Returns the thumbnail base dir, independent of the thumbnail size
     */
    static QString thumbnailBaseDir();

    /**
     * Sets the thumbnail base dir, useful for unit-testing
     */
    static void setThumbnailBaseDir(const QString&);

    /**
     * Returns the thumbnail base dir, for the @p group
     */
    static QString thumbnailBaseDir(ThumbnailGroup::Enum group);

    /**
     * Delete the thumbnail for the @p url
     */
    static void deleteImageThumbnail(const KUrl& url);

    /**
     * Move a thumbnail to match a file move
     */
    static void moveThumbnail(const KUrl& oldUrl, const KUrl& newUrl);

    /**
     * Returns true if all thumbnails have been written to disk. Useful for
     * unit-testing.
     */
    static bool isThumbnailWriterEmpty();

Q_SIGNALS:
    /**
     * Emitted when the thumbnail for the @p item has been loaded
     */
    void thumbnailLoaded(const KFileItem& item, const QPixmap&, const QSize&);

    void thumbnailLoadingFailed(const KFileItem& item);

    /**
     * Queue is empty
     */
    void finished();

protected:
    virtual void slotResult(KJob *job);

private Q_SLOTS:
    void determineNextIcon();
    void slotGotPreview(const KFileItem&, const QPixmap&);
    void checkThumbnail();
    void thumbnailReady(const QImage&, const QSize&);
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

    // Thumbnail group
    ThumbnailGroup::Enum mThumbnailGroup;

    ThumbnailGenerator* mThumbnailGenerator;
    QPointer<ThumbnailGenerator> mPreviousThumbnailGenerator;

    QStringList mPreviewPlugins;

    void createNewThumbnailGenerator();
    void abortSubjob();
    void startCreatingThumbnail(const QString& path);

    void emitThumbnailLoaded(const QImage& img, const QSize& size);

    QImage loadThumbnailFromCache() const;
};

} // namespace
#endif

