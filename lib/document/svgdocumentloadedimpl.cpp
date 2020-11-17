// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "svgdocumentloadedimpl.h"

// Qt
#include <QSvgRenderer>

// KF

// Local
#include "gwenview_lib_debug.h"

namespace Gwenview
{

struct SvgDocumentLoadedImplPrivate
{
    QByteArray mRawData;
    QSvgRenderer* mRenderer;
};

SvgDocumentLoadedImpl::SvgDocumentLoadedImpl(Document* document, const QByteArray& data)
: AbstractDocumentImpl(document)
, d(new SvgDocumentLoadedImplPrivate)
{
    d->mRawData = data;
    d->mRenderer = new QSvgRenderer(this);
}

SvgDocumentLoadedImpl::~SvgDocumentLoadedImpl()
{
    delete d;
}

void SvgDocumentLoadedImpl::init()
{
    d->mRenderer->load(d->mRawData);
    setDocumentImageSize(d->mRenderer->defaultSize());
    emit loaded();
}

Document::LoadingState SvgDocumentLoadedImpl::loadingState() const
{
    return Document::Loaded;
}

void SvgDocumentLoadedImpl::setImage(const QImage&)
{
    qCWarning(GWENVIEW_LIB_LOG) << "Should not be called";
}

QByteArray SvgDocumentLoadedImpl::rawData() const
{
    return d->mRawData;
}

QSvgRenderer* SvgDocumentLoadedImpl::svgRenderer() const
{
    return d->mRenderer;
}

} // namespace
