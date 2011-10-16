// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef SVGVIEWADAPTER_H
#define SVGVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGraphicsWidget>

// KDE

// Local
#include <lib/documentview/abstractdocumentviewadapter.h>

class QSvgRenderer;

namespace Gwenview {

class AbstractImageView : public QGraphicsWidget {
public:
    AbstractImageView(QGraphicsItem* parent = 0);

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/);

	qreal zoom() const;

	void setZoom(qreal zoom, const QPointF& /*center*/);

protected:
	virtual void updateCache() = 0;
	qreal mZoom;
	QPixmap mCachePix;
};

class SvgImageView : public AbstractImageView {
public:
    SvgImageView(QGraphicsItem* parent = 0);

	void loadFromDocument(Document::Ptr doc);

	QSize defaultSize() const;

protected:
	void updateCache();

private:
	QSvgRenderer* mRenderer;
};

struct SvgViewAdapterPrivate;
class GWENVIEWLIB_EXPORT SvgViewAdapter : public AbstractDocumentViewAdapter {
	Q_OBJECT
public:
	SvgViewAdapter();
	~SvgViewAdapter();

	virtual QCursor cursor() const;

	virtual void setCursor(const QCursor&);

	virtual void setDocument(Document::Ptr);

	virtual Document::Ptr document() const;

	virtual MimeTypeUtils::Kind kind() const { return MimeTypeUtils::KIND_SVG_IMAGE; }

	virtual bool canZoom() const { return true; }

	virtual void setZoomToFit(bool);

	virtual bool zoomToFit() const;

	virtual qreal zoom() const;

	virtual void setZoom(qreal /*zoom*/, const QPointF& /*center*/ = QPointF(-1, -1));

	virtual qreal computeZoomToFit() const;
	virtual qreal computeZoomToFitWidth() const;
	virtual qreal computeZoomToFitHeight() const;

protected:
	virtual bool eventFilter(QObject*, QEvent*);

private Q_SLOTS:
	void loadFromDocument();

private:
	SvgViewAdapterPrivate* const d;
};


} // namespace

#endif /* SVGVIEWADAPTER_H */
