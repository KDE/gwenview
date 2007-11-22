/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include "gwenviewlib_export.h"

// Qt
#include <QAbstractScrollArea>

namespace Gwenview {

class AbstractImageViewTool;
class ImageScaler;

class ImageViewPrivate;
class GWENVIEWLIB_EXPORT ImageView : public QAbstractScrollArea {
	Q_OBJECT
public:
	enum AlphaBackgroundMode {
		AlphaBackgroundCheckBoard,
		AlphaBackgroundSolid
	};

	ImageView(QWidget* parent);
	~ImageView();

	void setCurrentTool(AbstractImageViewTool*);
	AbstractImageViewTool* currentTool() const;

	void setAlphaBackgroundMode(AlphaBackgroundMode mode);

	void setAlphaBackgroundColor(const QColor& color);

	void setImage(const QImage& image);

	QImage image() const;

	void setZoom(qreal zoom);

	/**
	 * Change zoom level, making sure the view is centered on the center point
	 * @param zoom the zoom level
	 * @param center the center point, in image coordinates
	 */
	void setZoom(qreal zoom, const QPoint& center);

	qreal zoom() const;

	bool zoomToFit() const;

	QPoint imageOffset() const;

	QPoint mapToViewport(const QPoint& src);
	QPoint mapToImage(const QPoint& src);

	QRect mapToViewport(const QRect& src);
	QRect mapToImage(const QRect& src);

Q_SIGNALS:
	void zoomChanged();

public Q_SLOTS:
	void setZoomToFit(bool on);

	void updateImageRect(const QRect&);

protected:
	virtual void paintEvent(QPaintEvent*);

	virtual void resizeEvent(QResizeEvent*);

	virtual void scrollContentsBy(int dx, int dy);

	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);

private Q_SLOTS:
	void updateFromScaler(int left, int top, const QImage& image);

private:
	void updateScrollBars();
	void startScaler();

	ImageViewPrivate* const d;
};

} // namespace

#endif /* IMAGEVIEW_H */
