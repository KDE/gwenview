// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "videodocumentloadedimpl.h"

// Qt

// KF

// Local
#include "gwenview_lib_debug.h"

namespace Gwenview
{
struct VideoDocumentLoadedImplPrivate {
};

VideoDocumentLoadedImpl::VideoDocumentLoadedImpl(Document *document)
    : AbstractDocumentImpl(document)
    , d(new VideoDocumentLoadedImplPrivate)
{
}

VideoDocumentLoadedImpl::~VideoDocumentLoadedImpl()
{
    delete d;
}

void VideoDocumentLoadedImpl::init()
{
    Q_EMIT loaded();
}

Document::LoadingState VideoDocumentLoadedImpl::loadingState() const
{
    return Document::Loaded;
}

void VideoDocumentLoadedImpl::setImage(const QImage &)
{
    qCWarning(GWENVIEW_LIB_LOG) << "Should not be called";
}

} // namespace
