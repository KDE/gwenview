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
#ifndef IMAGESCALER_H
#define IMAGESCALER_H

// Qt
#include <QObject>

// KDE

// local
#include <lib/gwenviewlib_export.h>
#include <document/document.h>

class QImage;
class QRect;
class QRegion;

namespace Gwenview
{

class Document;

struct ImageScalerPrivate;
class GWENVIEWLIB_EXPORT ImageScaler : public QObject
{
    Q_OBJECT
public:
    explicit ImageScaler(QObject* parent = nullptr);
    ~ImageScaler();
    void setDocument(Document::Ptr);
    void setZoom(qreal);
    void setDestinationRegion(const QRegion&);

    void setTransformationMode(Qt::TransformationMode);

Q_SIGNALS:
    void scaledRect(int left, int top, const QImage&);

private:
    ImageScalerPrivate * const d;
    void scaleRect(const QRect&);

private Q_SLOTS:
    void doScale();
};

} // namespace

#endif /* IMAGESCALER_H */
