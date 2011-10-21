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
// Self
#include "imageviewadapter.moc"

// Qt
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTimer>

// KDE
#include <kurl.h>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/imagescaler.h>
#include <lib/imageview.h>
#include <lib/scrolltool.h>
#include <lib/document/documentfactory.h>

namespace Gwenview {

class TileArray {
public:
	TileArray()
	: mZoom(0)
	{}

	enum Which {
		All,
		Missing
	};
	
	void paint(QPainter* painter, const QRectF& dstRect) {
		Q_FOREACH(const TileId& id, findTileIdsForRect(dstRect, All)) {
			ImageHash::ConstIterator it = mImageForTileId.find(id);
			if (it != mImageForTileId.end()) {
				painter->drawImage(id.pos(), it.value());;
			}
		}
	}

	TileIdList findTileIdsForRect(const QRectF& dstRect, Which which) {
		return TileIdList();
	}

	void setZoom(qreal zoom) {
		mZoom = zoom;
		mImageForTileId.clear();
	}

	qreal zoom() const {
		return mZoom;
	}

	void setImageForTileId(const TileId& id, const QImage& image) {
		mImageForTileId.insert(id, image);
	}

private:
	typedef QHash<TileId, QImage> ImageHash;
	QHash<TileId, QImage> mImageForTileId;
	qreal mZoom;
};

struct RasterImageViewPrivate {
	RasterImageView* q;
	RasterItem* mItem;
	ImageScaler* mScaler;
	QTimer* mUpdateTimer;

	TileIdList mUpdateQueue;

	TileArray mCurrentTileArray;
	TileArray mOldTileArray;
	void startAnimationIfNecessary();
	void scheduleUpdate(const TileIdList&);
	void cancelUpdate();
};

//// RasterItem ////
class RasterItem : public QGraphicsObject {
public:
    RasterItem(RasterImageView* view)
	: mView(view)
	{
		setFlag(ItemUsesExtendedStyleOption);
		setFlag(ItemSendsGeometryChanges);
	}
	
	virtual QRectF boundingRect() const {
		return QRectF(
			QPointF(0, 0),
			mView->documentSize() * scale());
	}

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) {
		TileIdList ids = mView->d->mCurrentTileArray.findTileIdsForRect(option->exposedRect, TileArray::Missing);
		if (!ids.isEmpty()) {
			mView->d->scheduleUpdate(ids);
			mView->d->mOldTileArray.paint(painter, option->exposedRect);
		}
		mView->d->mCurrentTileArray.paint(painter, option->exposedRect);
	}

	virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) {
		if (change == ItemScaleHasChanged) {
			onItemScaleHasChanged();
		}
		return QGraphicsObject::itemChange(change, value);
	}

private:
	RasterImageView* mView;

	void onItemScaleHasChanged() {
		mView->d->cancelUpdate();
		qSwap(mView->d->mCurrentTileArray, mView->d->mOldTileArray);
		mView->d->mCurrentTileArray.setZoom(scale());
	}
};

//// RasterImageViewPrivate ////
void RasterImageViewPrivate::startAnimationIfNecessary() {
}

void RasterImageViewPrivate::scheduleUpdate(const QList& tiles) {
	mUpdateQueue += tiles;
	if (!mUpdateTimer->isActive()) {
		mUpdateTimer->start();
	}
}

void RasterImageViewPrivate::cancelUpdate() {
	mUpdateQueue.clear();
	mUpdateTimer->stop();
}

//// RasterImageView ////
RasterImageView::RasterImageView(QGraphicsItem* parent)
: AbstractImageView(parent)
, d(new RasterImageViewPrivate) {
	d->q = this;
	d->mItem = new RasterItem(this);
	setChildItem(d->mItem);

	d->mScaler = new ImageScaler(this);
	connect(d->mScaler, SIGNAL(scaled(TileId,QImage)),
		SLOT(updateFromScaler(TileId,QImage)) );

	d->mUpdateTimer = new QTimer(this);
	d->mUpdateTimer->setInterval(500);
	d->mUpdateTimer->setSingleShot(true);
	connect(d->mUpdateTimer, SIGNAL(timeout()),
		SLOT(processUpdates()));
}

RasterImageView::~RasterImageView() {
	delete d;
}

void RasterImageView::loadFromDocument() {
	Document::Ptr doc = document();
	connect(doc.data(), SIGNAL(metaInfoLoaded(KUrl)),
		SLOT(slotDocumentMetaInfoLoaded()) );
	connect(doc.data(), SIGNAL(isAnimatedUpdated()),
		SLOT(slotDocumentIsAnimatedUpdated()) );

	const Document::LoadingState state = doc->loadingState();
	if (state == Document::MetaInfoLoaded || state == Document::Loaded) {
		slotDocumentMetaInfoLoaded();
	}
}


void RasterImageView::slotDocumentMetaInfoLoaded() {
	if (document()->size().isValid()) {
		finishSetDocument();
	} else {
		// Could not retrieve image size from meta info, we need to load the
		// full image now.
		connect(document().data(), SIGNAL(loaded(KUrl)),
			SLOT(finishSetDocument()) );
		document()->startLoadingFullImage();
	}
}


void RasterImageView::finishSetDocument() {
	if (!document()->size().isValid()) {
		kError() << "No valid image size available, this should not happen!";
		return;
	}

	d->mScaler->setDocument(document());

	connect(document().data(), SIGNAL(imageRectUpdated(QRect)),
		SLOT(updateImageRect(QRect)) );

	if (zoomToFit()) {
		setZoom(computeZoomToFit());
	} else {
		QRect rect(QPoint(0, 0), document()->size());
		updateImageRect(rect);
	}

	d->startAnimationIfNecessary();
	update();
}

void RasterImageView::updateImageRect(const QRect& imageRect) {
	QRect visibleRect = mapRectFromItem(d->mItem, imageRect)
		.intersected(boundingRect());
	if (visibleRect.isEmpty()) {
		return;
	}

	if (zoomToFit()) {
		setZoom(computeZoomToFit());
	}
	TileIds ids = d->mCurrentTileArray.findTileIdsForRect(visibleRect, TileArray::All);
	d->scheduleUpdate(ids);
}

void RasterImageView::slotDocumentIsAnimatedUpdated() {
	d->startAnimationIfNecessary();
}

void RasterImageView::processUpdates() {
	Q_ASSERT(!d->mUpdateQueue.isEmpty());
	TileId id = d->mUpdateQueue.takeFirst();
	d->mScaler->setTileId(id);
	if (!d->mUpdateQueue.isEmpty()) {
		d->mUpdateTimer->start();
	}
}

#if 0
void RasterImageView::updateBuffer(const QRegion& region) {
	d->mScaler->setZoom(zoom());
	if (region.isEmpty()) {
		d->setScalerRegionToVisibleRect();
	} else {
		d->mScaler->setDestinationRegion(region);
	}
}

void RasterImageView::updateFromScaler(int zoomedImageLeft, int zoomedImageTop, const QImage& image) {
	int viewportLeft = zoomedImageLeft - scrollPos().x();
	int viewportTop = zoomedImageTop - scrollPos().y();
	{
		QPainter painter(&buffer());
		/*
		if (d->mDocument->hasAlphaChannel()) {
			d->drawAlphaBackground(
				&painter, QRect(viewportLeft, viewportTop, image.width(), image.height()),
				QPoint(zoomedImageLeft, zoomedImageTop)
				);
		} else {
			painter.setCompositionMode(QPainter::CompositionMode_Source);
		}
		painter.drawImage(viewportLeft, viewportTop, image);
		*/
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(viewportLeft, viewportTop, image);
	}
	update();
}
#endif
void RasterImageView::updateFromScaler(const TileId& id, const QImage& image) {
	if (id.zoom == d->mCurrentTileArray.zoom()) {
		setImageForTileId(id, image);
	}
}

//// ImageViewAdapter ////
struct ImageViewAdapterPrivate {
	ImageViewAdapter* that;
	RasterImageView* mView;
	/*
	ScrollTool* mScrollTool;

	void setupScrollTool() {
		mScrollTool = new ScrollTool(mView);
		mView->setDefaultTool(mScrollTool);
	}
	*/
};


ImageViewAdapter::ImageViewAdapter()
: d(new ImageViewAdapterPrivate) {
	d->that = this;
	/*
	d->mView = new ImageView;
	QGraphicsProxyWidget* proxyWidget = new QGraphicsProxyWidget;
	proxyWidget->setWidget(d->mView);
	setWidget(proxyWidget);
	d->setupScrollTool();

	*/
	d->mView = new RasterImageView;
	connect(d->mView, SIGNAL(zoomChanged(qreal)), SIGNAL(zoomChanged(qreal)) );
	connect(d->mView, SIGNAL(zoomToFitChanged(bool)), SIGNAL(zoomToFitChanged(bool)) );
	setWidget(d->mView);
}


ImageViewAdapter::~ImageViewAdapter() {
	delete d;
}


ImageView* ImageViewAdapter::imageView() const {
	// FIXME: QGV
	//return d->mView;
	return 0;
}


QCursor ImageViewAdapter::cursor() const {
	return d->mView->cursor();
}


void ImageViewAdapter::setCursor(const QCursor& cursor) {
	d->mView->setCursor(cursor);
}


void ImageViewAdapter::setDocument(Document::Ptr doc) {
	d->mView->setDocument(doc);

	connect(doc.data(), SIGNAL(loadingFailed(KUrl)), SLOT(slotLoadingFailed()) );
	if (doc->loadingState() == Document::LoadingFailed) {
		slotLoadingFailed();
	}
}


qreal ImageViewAdapter::zoom() const {
	return d->mView->zoom();
}


void ImageViewAdapter::setZoomToFit(bool on) {
	d->mView->setZoomToFit(on);
}


bool ImageViewAdapter::zoomToFit() const {
	return d->mView->zoomToFit();
}


void ImageViewAdapter::setZoom(qreal zoom, const QPointF& center) {
	d->mView->setZoom(zoom, center);
}


qreal ImageViewAdapter::computeZoomToFit() const {
	return d->mView->computeZoomToFit();
}


Document::Ptr ImageViewAdapter::document() const {
	return d->mView->document();
}


void ImageViewAdapter::slotLoadingFailed() {
	d->mView->setDocument(Document::Ptr());
}


void ImageViewAdapter::loadConfig() {
	// FIXME: QGV
	/*
	d->mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
	d->mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
	d->mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
	*/
}

} // namespace
