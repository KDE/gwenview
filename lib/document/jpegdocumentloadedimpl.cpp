// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "jpegdocumentloadedimpl.h"

// Qt
#include <QImage>
#include <QIODevice>

// KF

// Local
#include "jpegcontent.h"

namespace Gwenview
{

struct JpegDocumentLoadedImplPrivate
{
    JpegContent* mJpegContent;
};

JpegDocumentLoadedImpl::JpegDocumentLoadedImpl(Document* doc, JpegContent* jpegContent)
: DocumentLoadedImpl(doc, QByteArray() /* rawData */)
, d(new JpegDocumentLoadedImplPrivate)
{
    Q_ASSERT(jpegContent);
    d->mJpegContent = jpegContent;
}

JpegDocumentLoadedImpl::~JpegDocumentLoadedImpl()
{
    delete d->mJpegContent;
    delete d;
}

bool JpegDocumentLoadedImpl::saveInternal(QIODevice* device, const QByteArray& format)
{
    if (format == "jpeg") {
        if (!d->mJpegContent->thumbnail().isNull()) {
            QImage thumbnail = document()->image().scaled(128, 128, Qt::KeepAspectRatio);
            d->mJpegContent->setThumbnail(thumbnail);
        }

        bool ok = d->mJpegContent->save(device);
        if (!ok) {
            setDocumentErrorString(d->mJpegContent->errorString());
        }
        return ok;
    } else {
        return DocumentLoadedImpl::saveInternal(device, format);
    }
}

void JpegDocumentLoadedImpl::setImage(const QImage& image)
{
    d->mJpegContent->setImage(image);
    DocumentLoadedImpl::setImage(image);
}

void JpegDocumentLoadedImpl::applyTransformation(Orientation orientation)
{
    DocumentLoadedImpl::applyTransformation(orientation);

    // Apply Exif transformation first to normalize image
    d->mJpegContent->transform(d->mJpegContent->orientation());
    d->mJpegContent->resetOrientation();

    d->mJpegContent->transform(orientation);
}

QByteArray JpegDocumentLoadedImpl::rawData() const
{
    return d->mJpegContent->rawData();
}

} // namespace
