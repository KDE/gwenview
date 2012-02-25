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
#ifndef CROPTOOL_H
#define CROPTOOL_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/documentview/abstractrasterimageviewtool.h>

class QRect;

namespace Gwenview
{

class AbstractImageOperation;
class ImageView;

struct CropToolPrivate;
class GWENVIEWLIB_EXPORT CropTool : public AbstractRasterImageViewTool
{
    Q_OBJECT
public:
    CropTool(RasterImageView* parent);
    ~CropTool();

    void setCropRatio(double ratio);

    void setRect(const QRect&);
    QRect rect() const;

    virtual void paint(QPainter*);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent*);

    virtual void toolActivated();
    virtual void toolDeactivated();

    virtual QWidget* widget() const;

public Q_SLOTS:
    void onWidgetSlidedIn(); // reimp

Q_SIGNALS:
    void rectUpdated(const QRect&);
    void done();
    void imageOperationRequested(AbstractImageOperation*);

private Q_SLOTS:
    void slotCropRequested();

private:
    CropToolPrivate* const d;
};

} // namespace

#endif /* CROPTOOL_H */
