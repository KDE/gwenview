// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2013 Aurélien Gâteau <agateau@kde.org>

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
#ifndef DOCUMENT_P_H
#define DOCUMENT_P_H

// STL
#include <memory>

// Local
#include <document/documentjob.h>
#include <imagemetainfomodel.h>

// KF

// Qt
#include <QFutureWatcher>
#include <QImage>
#include <QPointer>
#include <QQueue>
#include <QUndoStack>
#include <QUrl>

#include "lib/renderingintent.h"

namespace Exiv2
{
class Image;
}

namespace Gwenview
{
using DocumentJobQueue = QQueue<DocumentJob *>;
struct DocumentPrivate {
    Document *q;
    AbstractDocumentImpl *mImpl;
    QUrl mUrl;
    bool mKeepRawData;
    QPointer<DocumentJob> mCurrentJob;
    DocumentJobQueue mJobQueue;

    /**
     * @defgroup imagedata should be reset in reload()
     * @{
     */
    QSize mSize;
    QImage mImage;
    QImage mColorCorrectedImage;
    QMap<int, QImage> mDownSampledImageMap;
    std::unique_ptr<Exiv2::Image> mExiv2Image;
    MimeTypeUtils::Kind mKind;
    QByteArray mFormat;
    ImageMetaInfoModel mImageMetaInfoModel;
    QUndoStack mUndoStack;
    QString mErrorString;

    Cms::Profile::Ptr mCmsProfile;
    bool mApplyDisplayTransform = true;
    cmsUInt32Number mRenderingIntent = INTENT_PERCEPTUAL;
    /** @} */

    void scheduleImageLoading(int invertedZoom);
    void scheduleImageDownSampling(int invertedZoom);
    void scheduleImageColorCorrection();
    void downSampleImage(int invertedZoom);
    void colorCorrectImage(QImage const &);
};

class DownSamplingJob : public DocumentJob
{
    Q_OBJECT
public:
    DownSamplingJob(int invertedZoom)
        : mInvertedZoom(invertedZoom)
    {
    }

    void doStart() override;

    int mInvertedZoom;
};

class ColorCorrectionJob : public DocumentJob
{
    Q_OBJECT
public:
    ColorCorrectionJob();
    ~ColorCorrectionJob();

    void doStart() override;

private:
    QFutureWatcher<QImage> mWatcher;
};

} // namespace

#endif /* DOCUMENT_P_H */
