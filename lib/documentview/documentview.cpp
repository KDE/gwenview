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
#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QShortcut>
#include <QVBoxLayout>

// KDE
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpixmapsequence.h>
#include <kpixmapsequencewidget.h>
#include <kstandarddirs.h>
#include <kurl.h>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/documentview/messageviewadapter.h>
#include <lib/documentview/imageviewadapter.h>
#include <lib/documentview/svgviewadapter.h>
#include <lib/documentview/videoviewadapter.h>
#include <lib/gwenviewconfig.h>
#include <lib/mimetypeutils.h>
#include <lib/signalblocker.h>
#include <lib/widgetfloater.h>

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


struct DocumentViewPrivate {
	DocumentView* that;
	KActionCollection* mActionCollection;
	QCursor mZoomCursor;
	QCursor mPreviousCursor;

	KPixmapSequenceWidget* mLoadingIndicator;

	AbstractDocumentViewAdapter* mAdapter;
	QList<qreal> mZoomSnapValues;
	Document::Ptr mDocument;


	void setCurrentAdapter(AbstractDocumentViewAdapter* adapter) {
		Q_ASSERT(adapter);
		delete mAdapter;
		mAdapter = adapter;

		mAdapter->loadConfig();

		QObject::connect(mAdapter, SIGNAL(previousImageRequested()),
			that, SIGNAL(previousImageRequested()) );
		QObject::connect(mAdapter, SIGNAL(nextImageRequested()),
			that, SIGNAL(nextImageRequested()) );
		QObject::connect(mAdapter, SIGNAL(zoomInRequested(const QPoint&)),
			that, SLOT(zoomIn(const QPoint&)) );
		QObject::connect(mAdapter, SIGNAL(zoomOutRequested(const QPoint&)),
			that, SLOT(zoomOut(const QPoint&)) );

		that->layout()->addWidget(mAdapter->widget());

		if (mAdapter->canZoom()) {
			QObject::connect(mAdapter, SIGNAL(zoomChanged(qreal)),
				that, SLOT(slotZoomChanged(qreal)) );
		}
		mAdapter->installEventFilterOnViewWidgets(that);

		that->adapterChanged();
	}

	void setupZoomCursor() {
		QString path = KStandardDirs::locate("appdata", "cursors/zoom.png");
		QPixmap cursorPixmap = QPixmap(path);
		mZoomCursor = QCursor(cursorPixmap);
	}

	void setZoomCursor() {
		mAdapter->widget()->grabKeyboard();
		QCursor currentCursor = mAdapter->cursor();
		if (currentCursor.pixmap().cacheKey() != mZoomCursor.pixmap().cacheKey()) {
			mPreviousCursor = currentCursor;
		}
		mAdapter->setCursor(mZoomCursor);
	}

	void restoreCursor() {
		mAdapter->widget()->releaseKeyboard();
		mAdapter->setCursor(mPreviousCursor);
	}

	void setupLoadingIndicator() {
		KPixmapSequence sequence("process-working", 22);
		mLoadingIndicator = new KPixmapSequenceWidget;
		mLoadingIndicator->setSequence(sequence);
		mLoadingIndicator->setInterval(100);

		WidgetFloater* floater = new WidgetFloater(that);
		floater->setChildWidget(mLoadingIndicator);
	}


	void updateCaption() {
		QString caption;
		if (!mAdapter) {
			emit that->captionUpdateRequested(caption);
			return;
		}

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


	void setZoom(qreal zoom, const QPoint& center = QPoint(-1, -1)) {
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
		mLoadingIndicator->raise();
	}


	void hideLoadingIndicator() {
		if (!mLoadingIndicator) {
			return;
		}
		mLoadingIndicator->hide();
	}


	bool adapterMousePressEventFilter(QMouseEvent* event) {
		if (mAdapter->canZoom()) {
			if (event->modifiers() == Qt::ControlModifier) {
				// Ctrl + Left or right button => zoom in or out
				setZoomCursor();
				if (event->button() == Qt::LeftButton) {
					that->zoomIn(event->pos());
				} else if (event->button() == Qt::RightButton) {
					that->zoomOut(event->pos());
				}
				return true;
			} else if (event->button() == Qt::MidButton) {
				// Middle click => toggle zoom to fit
				that->setZoomToFit(!mAdapter->zoomToFit());
				return true;
			}
		}
		return false;
	}

	bool adapterMouseReleaseEventFilter(QMouseEvent* event) {
		if (mAdapter->canZoom() && event->modifiers() == Qt::ControlModifier) {
			// Eat the mouse release so that the svg view does not restore its
			// drag cursor on release: we want the zoom cursor to stay as long
			// as Control is held down.
			return true;
		}
		return false;
	}

	bool adapterMouseDoubleClickEventFilter(QMouseEvent* event) {
		if (event->modifiers() == Qt::NoModifier) {
			that->toggleFullScreenRequested();
			return true;
		}
		return false;
	}


	bool adapterWheelEventFilter(QWheelEvent* event) {
		if (mAdapter->canZoom() && event->modifiers() & Qt::ControlModifier) {
			// Ctrl + wheel => zoom in or out
			setZoomCursor();
			if (event->delta() > 0) {
				that->zoomIn(event->pos());
			} else {
				that->zoomOut(event->pos());
			}
			return true;
		}
		if (event->modifiers() == Qt::NoModifier
			&& GwenviewConfig::mouseWheelBehavior() == MouseWheelBehavior::Browse
			) {
			// Browse with mouse wheel
			if (event->delta() > 0) {
				that->previousImageRequested();
			} else {
				that->nextImageRequested();
			}
			return true;
		}
		return false;
	}


	bool adapterContextMenuEventFilter(QContextMenuEvent* event) {
		// Filter out context menu if Ctrl is down to avoid showing it when
		// zooming out with Ctrl + Right button
		if (event->modifiers() == Qt::ControlModifier) {
			return true;
		}
		return false;
	}

	bool adapterKeyPressEventFilter(QKeyEvent* event) {
		if (mAdapter->canZoom() && event->modifiers() == Qt::ControlModifier) {
			setZoomCursor();
		}
		return false;
	}

	bool adapterKeyReleaseEventFilter(QKeyEvent* event) {
		if (mAdapter->canZoom() && event->modifiers() != Qt::ControlModifier) {
			restoreCursor();
		}
		return false;
	}
};


DocumentView::DocumentView(QWidget* parent, KActionCollection* actionCollection)
: QWidget(parent)
, d(new DocumentViewPrivate) {
	d->that = this;
	d->mActionCollection = actionCollection;
	d->mLoadingIndicator = 0;
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	d->mAdapter = 0;
	d->setupZoomCursor();
	d->setCurrentAdapter(new MessageViewAdapter(this));
}


DocumentView::~DocumentView() {
	delete d;
}


AbstractDocumentViewAdapter* DocumentView::adapter() const {
	return d->mAdapter;
}


void DocumentView::createAdapterForDocument() {
	Q_ASSERT(d->mAdapter);
	const MimeTypeUtils::Kind documentKind = d->mDocument->kind();
	if (documentKind != MimeTypeUtils::KIND_UNKNOWN && documentKind == d->mAdapter->kind()) {
		// Do not reuse for KIND_UNKNOWN: we may need to change the message
		LOG("Reusing current adapter");
		return;
	}
	AbstractDocumentViewAdapter* adapter = 0;
	switch (documentKind) {
	case MimeTypeUtils::KIND_RASTER_IMAGE:
		adapter = new ImageViewAdapter(this);
		break;
	case MimeTypeUtils::KIND_SVG_IMAGE:
		adapter = new SvgViewAdapter(this);
		break;
	case MimeTypeUtils::KIND_VIDEO:
		adapter = new VideoViewAdapter(this);
		connect(adapter, SIGNAL(videoFinished()),
			SIGNAL(videoFinished()));
		break;
	case MimeTypeUtils::KIND_UNKNOWN:
		adapter = new MessageViewAdapter(this);
		static_cast<MessageViewAdapter*>(adapter)->setErrorMessage(i18n("Gwenview does not know how to display this kind of document"));
		break;
	default:
		kWarning() << "should not be called for documentKind=" << documentKind;
		adapter = new MessageViewAdapter(this);
		break;
	}

	d->setCurrentAdapter(adapter);
}


void DocumentView::openUrl(const KUrl& url) {
	if (d->mDocument) {
		disconnect(d->mDocument.data(), 0, this, 0);
	}
	d->mDocument = DocumentFactory::instance()->load(url);
	connect(d->mDocument.data(), SIGNAL(busyChanged(const KUrl&, bool)), SLOT(slotBusyChanged(const KUrl&, bool)));

	if (d->mDocument->loadingState() < Document::KindDetermined) {
		MessageViewAdapter* messageViewAdapter = qobject_cast<MessageViewAdapter*>(d->mAdapter);
		if (messageViewAdapter) {
			messageViewAdapter->setInfoMessage(QString());
		}
		d->showLoadingIndicator();
		connect(d->mDocument.data(), SIGNAL(kindDetermined(const KUrl&)),
			SLOT(finishOpenUrl()));
	} else {
		finishOpenUrl();
	}
}


void DocumentView::finishOpenUrl() {
	disconnect(d->mDocument.data(), SIGNAL(kindDetermined(const KUrl&)),
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
	connect(d->mDocument.data(), SIGNAL(loaded(const KUrl&)),
		SLOT(slotLoaded()) );
	connect(d->mDocument.data(), SIGNAL(loadingFailed(const KUrl&)),
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
	d->setCurrentAdapter(new MessageViewAdapter(this));
}


bool DocumentView::isEmpty() const {
	return d->mAdapter->kind() == MimeTypeUtils::KIND_UNKNOWN;
}


void DocumentView::slotLoaded() {
	d->hideLoadingIndicator();
	d->updateCaption();
	d->updateZoomSnapValues();
	emit completed();
}


void DocumentView::slotLoadingFailed() {
	d->hideLoadingIndicator();
	MessageViewAdapter* adapter = new MessageViewAdapter(this);
	adapter->setDocument(d->mDocument);
	QString message = i18n("Loading <filename>%1</filename> failed", d->mDocument->url().fileName());
	adapter->setErrorMessage(message, d->mDocument->errorString());
	d->setCurrentAdapter(adapter);
	emit completed();
}


void DocumentView::setZoomToFit(bool on) {
	d->mAdapter->setZoomToFit(on);
	if (!on) {
		d->mAdapter->setZoom(1.);
	}
}


void DocumentView::zoomActualSize() {
	d->uncheckZoomToFit();
	d->mAdapter->setZoom(1.);
}


void DocumentView::zoomIn(const QPoint& center) {
	qreal currentZoom = d->mAdapter->zoom();

	Q_FOREACH(qreal zoom, d->mZoomSnapValues) {
		if (zoom > currentZoom + REAL_DELTA) {
			d->setZoom(zoom, center);
			return;
		}
	}
}


void DocumentView::zoomOut(const QPoint& center) {
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


bool DocumentView::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::MouseButtonPress) {
		return d->adapterMousePressEventFilter(static_cast<QMouseEvent*>(event));
	} else if (event->type() == QEvent::MouseButtonRelease) {
		return d->adapterMouseReleaseEventFilter(static_cast<QMouseEvent*>(event));
	} else if (event->type() == QEvent::Resize) {
		d->updateZoomSnapValues();
	} else if (event->type() == QEvent::MouseButtonDblClick) {
		return d->adapterMouseDoubleClickEventFilter(static_cast<QMouseEvent*>(event));
	} else if (event->type() == QEvent::Wheel) {
		return d->adapterWheelEventFilter(static_cast<QWheelEvent*>(event));
	} else if (event->type() == QEvent::ContextMenu) {
		return d->adapterContextMenuEventFilter(static_cast<QContextMenuEvent*>(event));
	} else if (event->type() == QEvent::KeyPress) {
		return d->adapterKeyPressEventFilter(static_cast<QKeyEvent*>(event));
	} else if (event->type() == QEvent::KeyRelease) {
		return d->adapterKeyReleaseEventFilter(static_cast<QKeyEvent*>(event));
	}

	return false;
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


} // namespace
