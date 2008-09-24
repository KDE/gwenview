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

// Local
#include <lib/document/document.h>

namespace Gwenview {

class AbstractImageViewTool;

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

	/**
	 * Defines the tool to display on the view. if @p tool is 0, fallback to
	 * the default tool.
	 * @see setDefaultTool
	 */
	void setCurrentTool(AbstractImageViewTool* tool);
	AbstractImageViewTool* currentTool() const;

	/**
	 * This is the tool which the view will fallback to if one calls
	 * setCurrentTool(0)
	 */
	void setDefaultTool(AbstractImageViewTool*);
	AbstractImageViewTool* defaultTool() const;

	void setAlphaBackgroundMode(AlphaBackgroundMode mode);

	void setAlphaBackgroundColor(const QColor& color);

	void setEnlargeSmallerImages(bool value);

	void setDocument(Document::Ptr document);

	Document::Ptr document() const;

	/**
	 * Change zoom level, making sure the point at @p center stays at the
	 * same position after zooming, if possible.
	 * @param zoom the zoom level
	 * @param center the center point, in viewport coordinates
	 */
	void setZoom(qreal zoom, const QPoint& center = QPoint(-1, -1));

	qreal zoom() const;

	bool zoomToFit() const;

	QPoint imageOffset() const;

	QPoint mapToViewport(const QPoint& src) const;
	QPoint mapToImage(const QPoint& src) const;

	QRect mapToViewport(const QRect& src) const;
	QRect mapToImage(const QRect& src) const;

	QPointF mapToViewportF(const QPointF& src) const;
	QPointF mapToImageF(const QPointF& src) const;

	QRectF mapToViewportF(const QRectF& src) const;
	QRectF mapToImageF(const QRectF& src) const;

	qreal computeZoomToFit() const;
	qreal computeZoomToFitWidth() const;
	qreal computeZoomToFitHeight() const;

Q_SIGNALS:
	void zoomChanged(qreal);

public Q_SLOTS:
	void setZoomToFit(bool on);

protected:
	virtual void paintEvent(QPaintEvent*);

	virtual void resizeEvent(QResizeEvent*);

	virtual void scrollContentsBy(int dx, int dy);

	virtual void showEvent(QShowEvent*);
	virtual void hideEvent(QHideEvent*);

	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void wheelEvent(QWheelEvent*);
	virtual void keyPressEvent(QKeyEvent*);
	virtual void keyReleaseEvent(QKeyEvent*);

private Q_SLOTS:
	void slotDocumentMetaInfoLoaded();
	void slotDocumentIsAnimatedUpdated();

	/**
	 * This method performs the necessary adjustments to get the view ready to
	 * display the document set with setDocument(). It needs to be postponed
	 * because setDocument() can be called with a document which has not been
	 * loaded yet and whose size is unknown.
	 */
	void finishSetDocument();
	void updateFromScaler(int left, int top, const QImage& image);
	void updateImageRect(const QRect&);

private:
	void updateScrollBars();

	ImageViewPrivate* const d;
};

} // namespace

#endif /* IMAGEVIEW_H */
