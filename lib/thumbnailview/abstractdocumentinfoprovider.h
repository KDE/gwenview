// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
#ifndef ABSTRACTDOCUMENTINFOPROVIDER_H
#define ABSTRACTDOCUMENTINFOPROVIDER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>

// KDE

// Local
#include <lib/thumbnailgroup.h>

class QPixmap;
class QSize;

class QUrl;

namespace Gwenview
{

class GWENVIEWLIB_EXPORT AbstractDocumentInfoProvider : public QObject
{
    Q_OBJECT
public:
    explicit AbstractDocumentInfoProvider(QObject* parent = nullptr);

    /**
     * Returns true if the document is currently busy (loading, saving,
     * rotating...)
     */
    virtual bool isBusy(const QUrl &url) = 0;

    virtual bool isModified(const QUrl &url) = 0;

    virtual void thumbnailForDocument(const QUrl &url, ThumbnailGroup::Enum, QPixmap* outPix, QSize* outFullSize) const = 0;

Q_SIGNALS:
    void busyStateChanged(const QUrl& url, bool busy);
    void documentChanged(const QUrl& url);
};

} // namespace

#endif /* ABSTRACTDOCUMENTINFOPROVIDER_H */
