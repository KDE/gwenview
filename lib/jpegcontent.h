// vim: set tabstop=4 shiftwidth=4 expandtab
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
#ifndef JPEGCONTENT_H
#define JPEGCONTENT_H

// Local
#include <QByteArray>
#include <lib/gwenviewlib_export.h>
#include <lib/orientation.h>
class QImage;
class QSize;
class QString;
class QIODevice;

namespace Exiv2
{
class Image;
}

namespace Gwenview
{
class GWENVIEWLIB_EXPORT JpegContent
{
public:
    JpegContent();
    ~JpegContent();

    Orientation orientation() const;
    void resetOrientation();

    int dotsPerMeterX() const;
    int dotsPerMeterY() const;

    QSize size() const;

    QString comment() const;
    void setComment(const QString &);

    void transform(Orientation);

    QImage thumbnail() const;
    void setThumbnail(const QImage &);

    // Recreate raw data to represent image
    // Note: thumbnail must be updated separately
    void setImage(const QImage &image);

    bool load(const QString &file);
    bool loadFromData(const QByteArray &rawData);
    /**
     * Use this version of loadFromData if you already have an Exiv2::Image*
     */
    bool loadFromData(const QByteArray &rawData, Exiv2::Image *);
    bool save(const QString &file);
    bool save(QIODevice *);

    QByteArray rawData() const;

    QString errorString() const;

private:
    struct Private;
    Private *d;

    JpegContent(const JpegContent &) = delete;
    void operator=(const JpegContent &) = delete;
    void applyPendingTransformation();
    int dotsPerMeter(const QString &keyName) const;
};

} // namespace

#endif /* JPEGCONTENT_H */
