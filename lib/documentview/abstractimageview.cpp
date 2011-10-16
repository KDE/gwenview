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
// Self
#include "abstractimageview.moc"

// Local

// KDE

// Qt
#include <QPainter>

namespace Gwenview {

struct AbstractImageViewPrivate {
};

AbstractImageView::AbstractImageView(QGraphicsItem* parent)
: QGraphicsWidget(parent)
, mZoom(1)
, d(new AbstractImageViewPrivate) {
}

AbstractImageView::~AbstractImageView() {
	delete d;
}

void AbstractImageView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) {
	painter->drawPixmap(
		(size().width() - mCachePix.width()) / 2,
		(size().height() - mCachePix.height()) / 2,
		mCachePix);
}

qreal AbstractImageView::zoom() const {
	return mZoom;
}

void AbstractImageView::setZoom(qreal zoom, const QPointF& /*center*/) {
	mZoom = zoom;
	updateCache();
}

} // namespace
