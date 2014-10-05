/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#ifndef IMAGESCALERTEST_H
#define IMAGESCALERTEST_H

#include "../lib/imagescaler.h"

// Qt
#include <QImage>
#include <QPainter>

// KDE
#include <QDebug>

class ImageScalerClient : public QObject
{
    Q_OBJECT
public:
    ImageScalerClient(Gwenview::ImageScaler* scaler)
    {
        connect(scaler, SIGNAL(scaledRect(int, int, const QImage&)),
                SLOT(slotScaledRect(int, int, const QImage&)));
    }

    struct ImageInfo
    {
        int left;
        int top;
        QImage image;
    };
    QVector<ImageInfo> mImageInfoList;

    QImage createFullImage()
    {
        Q_ASSERT(mImageInfoList.size() > 0);
        QImage::Format format = mImageInfoList[0].image.format();

        int imageWidth = 0;
        int imageHeight = 0;
        Q_FOREACH(const ImageInfo & info, mImageInfoList) {
            int right = info.left + info.image.width();
            int bottom = info.top + info.image.height();
            imageWidth = qMax(imageWidth, right);
            imageHeight = qMax(imageHeight, bottom);
        }

        QImage image(imageWidth, imageHeight, format);
        image.fill(0);
        QPainter painter(&image);
        Q_FOREACH(const ImageInfo & info, mImageInfoList) {
            painter.drawImage(info.left, info.top, info.image);
        }
        return image;
    }

public Q_SLOTS:
    void slotScaledRect(int left, int top, const QImage& image)
    {
        ImageInfo info;
        info.left = left;
        info.top = top;
        info.image = image;

        mImageInfoList.append(info);
    }
};

class ImageScalerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testScaleFullImage();

    // FIXME Disabled for now, does not compile since ImageScaler::setImage() has
    // been replaced with ImageScaler::setDocument()
#if 0
    void testScalePartialImage();
    void testScaleFullImageTwoPasses();
    void testScaleFullImageTwoPasses_data();
    void testScaleThinArea();
    void testDontCrashWithoutImage();
    void testScaleDownBigImage();
#endif
};

#endif // IMAGESCALERTEST_H
