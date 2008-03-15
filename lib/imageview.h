/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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

#include <lib/gwenviewlib_export.h>

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

	/**
	 * Set the image to display in this view. Note that we pass a pointer, not
	 * a reference, to make it more explicit that we do not keep a copy of the
	 * image. In particular, if the image gets modified outside (for example
	 * because it is loaded progressively), the view will still points to the
	 * modified image: it won't detach its own copy.
	 */
	void setImage(const QImage* image);

	const QImage* image() const;

	void setZoom(qreal zoom);

	/**
	 * Change zoom level, making sure the point at @p center stays at the
	 * same position after zooming, if possible.
	 * @param zoom the zoom level
	 * @param center the center point, in viewport coordinates
	 */
	void setZoom(qreal zoom, const QPoint& center);

	qreal zoom() const;

	bool zoomToFit() const;

	QPoint imageOffset() const;

	QPoint mapToViewport(const QPoint& src);
	QPoint mapToImage(const QPoint& src);

	QRect mapToViewport(const QRect& src);
	QRect mapToImage(const QRect& src);

	qreal computeZoomToFit() const;

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
	virtual void wheelEvent(QWheelEvent*);
	virtual void keyPressEvent(QKeyEvent*);
	virtual void keyReleaseEvent(QKeyEvent*);

private Q_SLOTS:
	void updateFromScaler(int left, int top, const QImage& image);

private:
	void updateScrollBars();

	ImageViewPrivate* const d;
};

} // namespace

#endif /* IMAGEVIEW_H */
