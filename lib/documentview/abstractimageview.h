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
	AbstractImageView(QGraphicsItem* parent);
	~AbstractImageView();

	qreal zoom() const;

	virtual void setZoom(qreal zoom, const QPointF& center = QPointF(-1, -1));

	bool zoomToFit() const;

	virtual void setZoomToFit(bool value);

	virtual void setDocument(Document::Ptr doc);

	Document::Ptr document() const;

	qreal computeZoomToFit() const;

	virtual QSizeF documentSize() const;

	QSizeF visibleImageSize() const;

	/**
	 * If the image is smaller than the view, imageOffset is the distance from
	 * the topleft corner of the view to the topleft corner of the image.
	 * Neither x nor y can be negative.
	 */
	QPointF imageOffset() const;

	/**
	 * The scroll position, in zoomed image coordinates.
	 * x and y are always between 0 and (docsize * zoom - viewsize)
	 */
	QPointF scrollPos() const;

Q_SIGNALS:
	void zoomToFitChanged(bool);
	void zoomChanged(qreal);

protected:
	void setChildItem(QGraphicsItem*);
	virtual void loadFromDocument() = 0;
	virtual void onZoomChanged() = 0;
	/**
	 * Called when the offset changes.
	 * Note: to avoid multiple adjustments, this is not called if zoom changes!
	 */
	virtual void onImageOffsetChanged() = 0;
	/**
	 * Called when the scrollPos changes.
	 * Note: to avoid multiple adjustments, this is not called if zoom changes!
	 */
	virtual void onScrollPosChanged(const QPointF& oldPos) = 0;

	void resizeEvent(QGraphicsSceneResizeEvent* event);

	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

private:
	friend class AbstractImageViewPrivate;
	AbstractImageViewPrivate* const d;
};



} // namespace

#endif /* ABSTRACTIMAGEVIEW_H */
