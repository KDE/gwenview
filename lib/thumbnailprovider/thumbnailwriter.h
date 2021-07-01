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
#ifndef THUMBNAILWRITER_H
#define THUMBNAILWRITER_H

// Local

// KF

// Qt
#include <QHash>
#include <QMutex>
#include <QThread>

class QImage;

namespace Gwenview
{
/**
 * Store thumbnails to disk when done generating them
 */
class ThumbnailWriter : public QThread
{
    Q_OBJECT
public:
    // Return thumbnail if it has still not been stored
    QImage value(const QString &) const;

    bool isEmpty() const;

public Q_SLOTS:
    void queueThumbnail(const QString &, const QImage &);

protected:
    void run() override;

private:
    using Cache = QHash<QString, QImage>;
    Cache mCache;
    mutable QMutex mMutex;
};

} // namespace

#endif /* THUMBNAILWRITER_H */
