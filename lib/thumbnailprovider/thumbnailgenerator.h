// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

// Local
#include <lib/thumbnailgroup.h>

// KDE
#include <KFileItem>

// Qt
#include <QImage>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

namespace Gwenview
{

struct ThumbnailContext {
    QImage mImage;
    int mOriginalWidth;
    int mOriginalHeight;
    bool mNeedCaching;

    bool load(const QString &pixPath, int pixelSize);
};

class ThumbnailGenerator : public QThread
{
    Q_OBJECT
public:
    ThumbnailGenerator();

    void load(
        const QString& originalUri,
        time_t originalTime,
        KIO::filesize_t originalFileSize,
        const QString& originalMimeType,
        const QString& pixPath,
        const QString& thumbnailPath,
        ThumbnailGroup::Enum group);

    void cancel();

    QString originalUri() const;
    time_t originalTime() const;
    KIO::filesize_t originalFileSize() const;
    QString originalMimeType() const;
protected:
    virtual void run() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void done(const QImage&, const QSize&);
    void thumbnailReadyToBeCached(const QString& thumbnailPath, const QImage&);

private:
    bool testCancel();
    void cacheThumbnail();
    QImage mImage;
    QString mPixPath;
    QString mThumbnailPath;
    QString mOriginalUri;
    time_t mOriginalTime;
    KIO::filesize_t mOriginalFileSize;
    QString mOriginalMimeType;
    int mOriginalWidth;
    int mOriginalHeight;
    QMutex mMutex;
    QWaitCondition mCond;
    ThumbnailGroup::Enum mThumbnailGroup;
    bool mCancel;
};

} // namespace

#endif /* THUMBNAILGENERATOR_H */
