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
#include "documentview.moc"

// Qt
#include <QAbstractScrollArea>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWeakPointer>

// KDE
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmodifierkeyinfo.h>
#include <kstandarddirs.h>
#include <kurl.h>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/documentview/abstractrasterimageviewtool.h>
#include <lib/documentview/loadingindicator.h>
#include <lib/documentview/imageviewadapter.h>
#include <lib/documentview/messageviewadapter.h>
#include <lib/documentview/rasterimageview.h>
#include <lib/documentview/svgviewadapter.h>
#include <lib/documentview/videoviewadapter.h>
#include <lib/graphicshudwidget.h>
#include <lib/graphicswidgetfloater.h>
#include <lib/gwenviewconfig.h>
#include <lib/mimetypeutils.h>
#include <lib/signalblocker.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

static const qreal REAL_DELTA = 0.001;
static const qreal MAXIMUM_ZOOM_VALUE = qreal(DocumentView::MaximumZoom);

static const int COMPARE_MARGIN = 4;

struct DocumentViewPrivate {
	DocumentView* that;
	GraphicsHudWidget* mHud;
	KModifierKeyInfo* mModifierKeyInfo;
	QCursor mZoomCursor;
	QCursor mPreviousCursor;
	QWeakPointer<QPropertyAnimation> mMoveAnimation;

	LoadingIndicator* mLoadingIndicator;

	QScopedPointer<AbstractDocumentViewAdapter> mAdapter;
	QList<qreal> mZoomSnapValues;
	Document::Ptr mDocument;
	bool mCurrent;
	bool mCompareMode;

	void setCurrentAdapter(AbstractDocumentViewAdapter* adapter) {
		Q_ASSERT(adapter);
		mAdapter.reset(adapter);

		adapter->widget()->setParentItem(that);
		resizeAdapterWidget();

		if (adapter->canZoom()) {
			QObject::connect(adapter, SIGNAL(zoomChanged(qreal)),
				that, SLOT(slotZoomChanged(qreal)) );
			QObject::connect(adapter, SIGNAL(zoomToFitChanged(bool)),
				that, SIGNAL(zoomToFitChanged(bool)) );
		}

		QObject::connect(adapter, SIGNAL(scrollPosChanged()),
				that, SIGNAL(positionChanged()));

		adapter->loadConfig();

		adapter->widget()->installSceneEventFilter(that);
		if (mCurrent) {
			adapter->widget()->setFocus();
		}

		that->adapterChanged();
		that->positionChanged();
		if (adapter->canZoom()) {
			that->zoomToFitChanged(adapter->zoomToFit());
		}
		if (adapter->rasterImageView()) {
			QObject::connect(adapter->rasterImageView(), SIGNAL(currentToolChanged(AbstractRasterImageViewTool*)),
				that, SIGNAL(currentToolChanged(AbstractRasterImageViewTool*)));
		}
	}

	void setupZoomCursor() {
		QString path = KStandardDirs::locate("appdata", "cursors/zoom.png");
		QPixmap cursorPixmap = QPixmap(path);
		mZoomCursor = QCursor(cursorPixmap);
	}

	void setZoomCursor() {
		QCursor currentCursor = mAdapter.data()->cursor();
		if (currentCursor.pixmap().cacheKey() == mZoomCursor.pixmap().cacheKey()) {
			return;
		}
		mPreviousCursor = currentCursor;
		mAdapter.data()->setCursor(mZoomCursor);
	}

	void restoreCursor() {
		mAdapter.data()->setCursor(mPreviousCursor);
	}

	void setupLoadingIndicator() {
		mLoadingIndicator = new LoadingIndicator(that);
		GraphicsWidgetFloater* floater = new GraphicsWidgetFloater(that);
		floater->setChildWidget(mLoadingIndicator);
	}

	QToolButton* createHudButton(const QString& text, const char* iconName, bool showText) {
		QToolButton* button = new QToolButton;
		if (showText) {
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			button->setText(text);
		} else {
			button->setToolTip(text);
		}
		button->setIcon(SmallIcon(iconName));
		return button;
	}

	void setupHud() {
		QToolButton* trashButton = createHudButton(i18n("Trash"), "user-trash", false);
		QToolButton* deselectButton = createHudButton(i18n("Deselect"), "list-remove", true);

		QWidget* content = new QWidget;
		QHBoxLayout* layout = new QHBoxLayout(content);
		layout->setMargin(0);
		layout->setSpacing(4);
		layout->addWidget(trashButton);
		layout->addWidget(deselectButton);

		mHud = new GraphicsHudWidget(that);
		mHud->init(content, GraphicsHudWidget::OptionNone);
		GraphicsWidgetFloater* floater = new GraphicsWidgetFloater(that);
		floater->setChildWidget(mHud);
		floater->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

		QObject::connect(trashButton, SIGNAL(clicked()), that, SLOT(emitHudTrashClicked()));
		QObject::connect(deselectButton, SIGNAL(clicked()), that, SLOT(emitHudDeselectClicked()));

		mHud->hide();
	}

	void updateCaption() {
		QString caption;

		Document::Ptr doc = mAdapter->document();
		if (!doc) {
			emit that->captionUpdateRequested(caption);
			return;
		}

		caption = doc->url().fileName();
		QSize size = doc->size();
		if (size.isValid()) {
			caption +=
				QString(" - %1x%2")
					.arg(size.width())
					.arg(size.height());
			if (mAdapter->canZoom()) {
				int intZoom = qRound(mAdapter->zoom() * 100);
				caption += QString(" - %1%")
					.arg(intZoom);
			}
		}
		emit that->captionUpdateRequested(caption);
	}


	void uncheckZoomToFit() {
		if (mAdapter->zoomToFit()) {
			mAdapter->setZoomToFit(false);
		}
	}


	void setZoom(qreal zoom, const QPointF& center = QPointF(-1, -1)) {
		uncheckZoomToFit();
		zoom = qBound(that->minimumZoom(), zoom, MAXIMUM_ZOOM_VALUE);
		mAdapter->setZoom(zoom, center);
	}


	void updateZoomSnapValues() {
		qreal min = that->minimumZoom();

		mZoomSnapValues.clear();
		if (min < 1.) {
			mZoomSnapValues << min;
			for (qreal invZoom = 16.; invZoom > 1.; invZoom /= 2.) {
				qreal zoom = 1. / invZoom;
				if (zoom > min) {
					mZoomSnapValues << zoom;
				}
			}
		}
		for (qreal zoom = 1; zoom <= MAXIMUM_ZOOM_VALUE ; zoom += 1.) {
			mZoomSnapValues << zoom;
		}

		that->minimumZoomChanged(min);
	}


	void showLoadingIndicator() {
		if (!mLoadingIndicator) {
			setupLoadingIndicator();
		}
		mLoadingIndicator->show();
		mLoadingIndicator->setZValue(1);
	}


	void hideLoadingIndicator() {
		if (!mLoadingIndicator) {
			return;
		}
		mLoadingIndicator->hide();
	}

	void animate(QPropertyAnimation* anim) {
		QObject::connect(anim, SIGNAL(finished()),
			that, SLOT(slotAnimationFinished()));
		anim->setDuration(500);
		anim->start(QAbstractAnimation::DeleteWhenStopped);
	}

	void resizeAdapterWidget() {
		QRectF rect = QRectF(QPointF(0, 0), that->boundingRect().size());
		if (mCompareMode) {
			rect.adjust(COMPARE_MARGIN, COMPARE_MARGIN, -COMPARE_MARGIN, -COMPARE_MARGIN);
		}
		mAdapter->widget()->setGeometry(rect);
	}

	void adapterMousePressEvent(QGraphicsSceneMouseEvent* event) {
		if (mAdapter->canZoom()) {
			if (event->modifiers() == Qt::ControlModifier) {
				// Ctrl + Left or right button => zoom in or out
				if (event->button() == Qt::LeftButton) {
					that->zoomIn(event->pos());
				} else if (event->button() == Qt::RightButton) {
					that->zoomOut(event->pos());
				}
			} else if (event->button() == Qt::MidButton) {
				// Middle click => toggle zoom to fit
				that->setZoomToFit(!mAdapter->zoomToFit());
			}
		}
		QMetaObject::invokeMethod(that, "emitFocused", Qt::QueuedConnection);
	}
};


DocumentView::DocumentView(QGraphicsScene* scene)
: d(new DocumentViewPrivate) {
	setFlag(ItemIsFocusable);
	setFlag(ItemIsSelectable);
	setFlag(ItemClipsChildrenToShape);

	d->that = this;
	d->mLoadingIndicator = 0;
	d->mCurrent = false;
	d->mCompareMode = false;
	d->mModifierKeyInfo = new KModifierKeyInfo(this);
	connect(d->mModifierKeyInfo, SIGNAL(keyPressed(Qt::Key,bool)), SLOT(slotKeyPressed(Qt::Key,bool)));

	scene->addItem(this);

	d->setupZoomCursor();
	d->setupHud();
	d->setCurrentAdapter(new MessageViewAdapter);
}


DocumentView::~DocumentView() {
	delete d;
}


void DocumentView::createAdapterForDocument() {
	const MimeTypeUtils::Kind documentKind = d->mDocument->kind();
	if (d->mAdapter && documentKind == d->mAdapter->kind() && documentKind != MimeTypeUtils::KIND_UNKNOWN) {
		// Do not reuse for KIND_UNKNOWN: we may need to change the message
		LOG("Reusing current adapter");
		return;
	}
	AbstractDocumentViewAdapter* adapter = 0;
	switch (documentKind) {
	case MimeTypeUtils::KIND_RASTER_IMAGE:
		adapter = new ImageViewAdapter;
		break;
	case MimeTypeUtils::KIND_SVG_IMAGE:
		adapter = new SvgViewAdapter;
		break;
	case MimeTypeUtils::KIND_VIDEO:
		adapter = new VideoViewAdapter;
		connect(adapter, SIGNAL(videoFinished()),
			SIGNAL(videoFinished()));
		break;
	case MimeTypeUtils::KIND_UNKNOWN:
		adapter = new MessageViewAdapter;
		static_cast<MessageViewAdapter*>(adapter)->setErrorMessage(i18n("Gwenview does not know how to display this kind of document"));
		break;
	default:
		kWarning() << "should not be called for documentKind=" << documentKind;
		adapter = new MessageViewAdapter;
		break;
	}

	d->setCurrentAdapter(adapter);
}


void DocumentView::openUrl(const KUrl& url) {
	if (d->mDocument) {
		disconnect(d->mDocument.data(), 0, this, 0);
	}
	d->mDocument = DocumentFactory::instance()->load(url);
	connect(d->mDocument.data(), SIGNAL(busyChanged(KUrl,bool)), SLOT(slotBusyChanged(KUrl,bool)));

	if (d->mDocument->loadingState() < Document::KindDetermined) {
		MessageViewAdapter* messageViewAdapter = qobject_cast<MessageViewAdapter*>(d->mAdapter.data());
		if (messageViewAdapter) {
			messageViewAdapter->setInfoMessage(QString());
		}
		d->showLoadingIndicator();
		connect(d->mDocument.data(), SIGNAL(kindDetermined(KUrl)),
			SLOT(finishOpenUrl()));
	} else {
		finishOpenUrl();
	}
}


void DocumentView::finishOpenUrl() {
	disconnect(d->mDocument.data(), SIGNAL(kindDetermined(KUrl)),
		this, SLOT(finishOpenUrl()));
	if (d->mDocument->loadingState() < Document::KindDetermined) {
		kWarning() << "d->mDocument->loadingState() < Document::KindDetermined, this should not happen!";
		return;
	}

	if (d->mDocument->loadingState() == Document::LoadingFailed) {
		slotLoadingFailed();
		return;
	}
	createAdapterForDocument();

	connect(d->mDocument.data(), SIGNAL(downSampledImageReady()),
		SLOT(slotLoaded()) );
	connect(d->mDocument.data(), SIGNAL(loaded(KUrl)),
		SLOT(slotLoaded()) );
	connect(d->mDocument.data(), SIGNAL(loadingFailed(KUrl)),
		SLOT(slotLoadingFailed()) );
	d->mAdapter->setDocument(d->mDocument);
	d->updateCaption();

	if (d->mDocument->loadingState() == Document::Loaded) {
		slotLoaded();
	}
}


void DocumentView::reset() {
	d->hideLoadingIndicator();
	if (d->mDocument) {
		disconnect(d->mDocument.data(), 0, this, 0);
		d->mDocument = 0;
	}
	d->setCurrentAdapter(new EmptyAdapter);
}


bool DocumentView::isEmpty() const {
	return qobject_cast<EmptyAdapter*>(d->mAdapter.data());
}


void DocumentView::loadAdapterConfig() {
	d->mAdapter->loadConfig();
}


RasterImageView* DocumentView::imageView() const {
	return d->mAdapter->rasterImageView();
}


void DocumentView::slotLoaded() {
	d->hideLoadingIndicator();
	d->updateCaption();
	d->updateZoomSnapValues();
	if (!d->mAdapter->zoomToFit()) {
		qreal min = minimumZoom();
		if (d->mAdapter->zoom() < min) {
			d->mAdapter->setZoom(min);
		}
	}
	emit completed();
}


void DocumentView::slotLoadingFailed() {
	d->hideLoadingIndicator();
	MessageViewAdapter* adapter = new MessageViewAdapter;
	adapter->setDocument(d->mDocument);
	QString message = i18n("Loading <filename>%1</filename> failed", d->mDocument->url().fileName());
	adapter->setErrorMessage(message, d->mDocument->errorString());
	d->setCurrentAdapter(adapter);
	emit completed();
}


bool DocumentView::canZoom() const {
	return d->mAdapter->canZoom();
}


void DocumentView::setZoomToFit(bool on) {
	if (on == d->mAdapter->zoomToFit()) {
		return;
	}
	d->mAdapter->setZoomToFit(on);
	if (!on) {
		d->mAdapter->setZoom(1.);
	}
}


bool DocumentView::zoomToFit() const {
	return d->mAdapter->zoomToFit();
}


void DocumentView::zoomActualSize() {
	d->uncheckZoomToFit();
	d->mAdapter->setZoom(1.);
}


void DocumentView::zoomIn(const QPointF& center) {
	qreal currentZoom = d->mAdapter->zoom();

	Q_FOREACH(qreal zoom, d->mZoomSnapValues) {
		if (zoom > currentZoom + REAL_DELTA) {
			d->setZoom(zoom, center);
			return;
		}
	}
}


void DocumentView::zoomOut(const QPointF& center) {
	qreal currentZoom = d->mAdapter->zoom();

	QListIterator<qreal> it(d->mZoomSnapValues);
	it.toBack();
	while (it.hasPrevious()) {
		qreal zoom = it.previous();
		if (zoom < currentZoom - REAL_DELTA) {
			d->setZoom(zoom, center);
			return;
		}
	}
}


void DocumentView::slotZoomChanged(qreal zoom) {
	d->updateCaption();
	zoomChanged(zoom);
}


void DocumentView::setZoom(qreal zoom) {
	d->setZoom(zoom);
}


qreal DocumentView::zoom() const {
	return d->mAdapter->zoom();
}

void DocumentView::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
	if (event->modifiers() == Qt::NoModifier) {
		toggleFullScreenRequested();
	}
}

void DocumentView::wheelEvent(QGraphicsSceneWheelEvent* event) {
	if (d->mAdapter->canZoom() && event->modifiers() & Qt::ControlModifier) {
		// Ctrl + wheel => zoom in or out
		if (event->delta() > 0) {
			zoomIn(event->pos());
		} else {
			zoomOut(event->pos());
		}
		return;
	}
	if (event->modifiers() == Qt::NoModifier
		&& GwenviewConfig::mouseWheelBehavior() == MouseWheelBehavior::Browse
		) {
		// Browse with mouse wheel
		if (event->delta() > 0) {
			previousImageRequested();
		} else {
			nextImageRequested();
		}
	}
}

void DocumentView::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
	// Filter out context menu if Ctrl is down to avoid showing it when
	// zooming out with Ctrl + Right button
	if (event->modifiers() != Qt::ControlModifier) {
		contextMenuRequested();
	}
}

void DocumentView::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) {
	if (!d->mCompareMode) {
		return;
	}
	if (d->mCurrent) {
		painter->save();
		painter->setBrush(Qt::NoBrush);
		painter->setPen(QPen(palette().highlight().color(), 2));
		painter->setRenderHint(QPainter::Antialiasing);
		QRectF selectionRect = boundingRect().adjusted(2, 2, -2, -2);
		painter->drawRoundedRect(selectionRect, 3, 3);
		painter->restore();
	}
}


void DocumentView::slotBusyChanged(const KUrl&, bool busy) {
	if (busy) {
		d->showLoadingIndicator();
	} else {
		d->hideLoadingIndicator();
	}
}


qreal DocumentView::minimumZoom() const {
	// There is no point zooming out less than zoomToFit, but make sure it does
	// not get too small either
	return qMax(0.001, qMin(double(d->mAdapter->computeZoomToFit()), 1.));
}


void DocumentView::setCompareMode(bool compare) {
	d->mCompareMode = compare;
	d->resizeAdapterWidget();
	if (compare) {
		d->mHud->show();
		d->mHud->setZValue(1);
	} else {
		d->mHud->hide();
	}
}


void DocumentView::setCurrent(bool value) {
	d->mCurrent = value;
	if (value) {
		d->mAdapter->widget()->setFocus();
	}
	update();
}


bool DocumentView::isCurrent() const {
	return d->mCurrent;
}


QPoint DocumentView::position() const {
	return d->mAdapter->scrollPos().toPoint();
}


void DocumentView::setPosition(const QPoint& pos) {
	d->mAdapter->setScrollPos(pos);
}


void DocumentView::slotKeyPressed(Qt::Key key, bool pressed)
{
	if (key == Qt::Key_Control) {
		if (pressed) {
			d->setZoomCursor();
		} else {
			d->restoreCursor();
		}
	}
}


Document::Ptr DocumentView::document() const {
	return d->mDocument;
}


KUrl DocumentView::url() const {
	Document::Ptr doc = d->mDocument;
	return doc ? doc->url() : KUrl();
}

void DocumentView::emitHudDeselectClicked() {
	hudDeselectClicked(this);
}

void DocumentView::emitHudTrashClicked() {
	hudTrashClicked(this);
}

void DocumentView::emitFocused() {
	focused(this);
}

void DocumentView::setGeometry(const QRectF& rect) {
	QGraphicsWidget::setGeometry(rect);
	d->resizeAdapterWidget();
}

void DocumentView::moveTo(const QRect& rect) {
	if (d->mMoveAnimation) {
		d->mMoveAnimation.data()->setEndValue(rect);
	} else {
		setGeometry(rect);
	}
}

void DocumentView::moveToAnimated(const QRect& rect) {
	QPropertyAnimation* anim = new QPropertyAnimation(this, "geometry");
	anim->setStartValue(geometry());
	anim->setEndValue(rect);
	d->animate(anim);
	d->mMoveAnimation = anim;
}

void DocumentView::fadeIn() {
	setOpacity(0);
	show();
	QPropertyAnimation* anim = new QPropertyAnimation(this, "opacity");
	anim->setStartValue(0.);
	anim->setEndValue(1.);
	d->animate(anim);
}

void DocumentView::fadeOut() {
	QPropertyAnimation* anim = new QPropertyAnimation(this, "opacity");
	anim->setStartValue(1.);
	anim->setEndValue(0.);
	d->animate(anim);
}

void DocumentView::slotAnimationFinished() {
	animationFinished(this);
}

bool DocumentView::sceneEventFilter(QGraphicsItem*, QEvent* event) {
	if (event->type() == QEvent::GraphicsSceneMousePress) {
		QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
		d->adapterMousePressEvent(mouseEvent);
	}
	return false;
}

AbstractRasterImageViewTool* DocumentView::currentTool() const {
	return imageView() ? imageView()->currentTool() : 0;
}

} // namespace
