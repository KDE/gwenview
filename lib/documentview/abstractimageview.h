// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef ABSTRACTIMAGEVIEW_H
#define ABSTRACTIMAGEVIEW_H

// Local
#include <lib/document/document.h>

// KDE

// Qt
#include <QGraphicsWidget>

namespace Gwenview {


class AbstractImageViewPrivate;
/**
 *
 */
class AbstractImageView : public QGraphicsWidget {
	Q_OBJECT
public:
    AbstractImageView(QGraphicsItem* parent = 0);
	~AbstractImageView();

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	qreal zoom() const;

	virtual void setZoom(qreal zoom, const QPointF& center = QPointF(-1, -1));

	bool zoomToFit() const;

	virtual void setZoomToFit(bool value);

	virtual void setDocument(Document::Ptr doc);

	Document::Ptr document() const;

	qreal computeZoomToFit() const;

	virtual QSizeF documentSize() const;

	QSizeF visibleImageSize() const;

	QRectF mapViewportToZoomedImage(const QRectF& viewportRect) const;

Q_SIGNALS:
	void zoomToFitChanged(bool);

protected:
	virtual void updateBuffer() = 0;
	void createBuffer();
	QPixmap& buffer();

	void resizeEvent(QGraphicsSceneResizeEvent* event);

private:
	AbstractImageViewPrivate* const d;
};



} // namespace

#endif /* ABSTRACTIMAGEVIEW_H */
