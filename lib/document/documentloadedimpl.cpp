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
#include "documentloadedimpl.h"

// Qt
#include <QByteArray>
#include <QImage>
#include <QImageWriter>
#include <QTransform>
#include <QDebug>
#include <QUrl>

// KDE

// Local
#include "documentjob.h"
#include "imageutils.h"
#include "savejob.h"

namespace Gwenview
{

struct DocumentLoadedImplPrivate
{
    QByteArray mRawData;
    bool mQuietInit;
};

DocumentLoadedImpl::DocumentLoadedImpl(Document* document, const QByteArray& rawData, bool quietInit)
: AbstractDocumentImpl(document)
, d(new DocumentLoadedImplPrivate)
{
    if (document->keepRawData()) {
        d->mRawData = rawData;
    }
    d->mQuietInit = quietInit;
}

DocumentLoadedImpl::~DocumentLoadedImpl()
{
    delete d;
}

void DocumentLoadedImpl::init()
{
    if (!d->mQuietInit) {
        emit imageRectUpdated(document()->image().rect());
        emit loaded();
    }
}

bool DocumentLoadedImpl::isEditable() const
{
    return true;
}

Document::LoadingState DocumentLoadedImpl::loadingState() const
{
    return Document::Loaded;
}

bool DocumentLoadedImpl::saveInternal(QIODevice* device, const QByteArray& format)
{
    QImageWriter writer(device, format);
    bool ok = writer.write(document()->image());
    if (ok) {
        setDocumentFormat(format);
    } else {
        setDocumentErrorString(writer.errorString());
    }
    return ok;
}

DocumentJob* DocumentLoadedImpl::save(const QUrl &url, const QByteArray& format)
{
    return new SaveJob(this, url, format);
}

AbstractDocumentEditor* DocumentLoadedImpl::editor()
{
    return this;
}

void DocumentLoadedImpl::setImage(const QImage& image)
{
    setDocumentImage(image);
    emit imageRectUpdated(image.rect());
}

void DocumentLoadedImpl::applyTransformation(Orientation orientation)
{
    QImage image = document()->image();
    QTransform matrix = ImageUtils::transformMatrix(orientation);
    image = image.transformed(matrix);
    setDocumentImage(image);
    emit imageRectUpdated(image.rect());
}

QByteArray DocumentLoadedImpl::rawData() const
{
    return d->mRawData;
}

} // namespace
