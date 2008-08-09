// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QMouseEvent>
#include <QVBoxLayout>

// KDE
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/emptyviewadapter.h>
#include <lib/imageviewadapter.h>
#include <lib/mimetypeutils.h>
#include <lib/signalblocker.h>
#include <lib/svgviewadapter.h>
#include <lib/zoomwidget.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

static const qreal REAL_DELTA = 0.001;
static const qreal MAXIMUM_ZOOM_VALUE = 16.;


typedef AbstractDocumentViewAdapter* (*AdapterCreator)(QWidget* parent);

// This macro makes it easy to declare a function whose prototype is
// AdapterCreator
#define DECLARE_ADAPTOR(className) \
static AbstractDocumentViewAdapter* create##className(QWidget* parent) { \
	LOG(""); \
	return new className(parent); \
}

DECLARE_ADAPTOR(ImageViewAdapter)
DECLARE_ADAPTOR(SvgViewAdapter)
DECLARE_ADAPTOR(EmptyViewAdapter)


struct DocumentViewPrivate {
	DocumentView* that;
	KActionCollection* mActionCollection;
	ZoomWidget* mZoomWidget;
	KAction* mZoomToFitAction;

	AbstractDocumentViewAdapter* mAdapter;
	AdapterCreator mAdapterCreator;
	QList<qreal> mZoomSnapValues;
	Document::Ptr mDocument;


	void createAdapter(AdapterCreator creator, AbstractDocumentViewAdapter* adapter = 0) {
		if (mAdapterCreator == creator) {
			LOG("Reusing current adapter");
			return;
		}

		delete mAdapter;
		mAdapterCreator = creator;
		mAdapter = !adapter ? mAdapterCreator(that) : adapter;

		QObject::connect(mAdapter, SIGNAL(resizeRequested(const QSize&)),
			that, SIGNAL(resizeRequested(const QSize&)) );
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
			mZoomWidget->show();
		} else {
			mZoomWidget->hide();
		}
		mAdapter->installEventFilterOnViewWidgets(that);
	}


	void setupZoomWidget() {
		mZoomWidget = new ZoomWidget;
		QObject::connect(mZoomWidget, SIGNAL(zoomChanged(qreal)),
			that, SLOT(slotZoomWidgetChanged(qreal)) );
	}

	void setupZoomActions() {
		mZoomToFitAction = new KAction(mActionCollection);
		mZoomToFitAction->setCheckable(true);
		mZoomToFitAction->setChecked(true);
		mZoomToFitAction->setText(i18n("Zoom to Fit"));
		mZoomToFitAction->setIcon(KIcon("zoom-fit-best"));
		mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));
		QObject::connect(mZoomToFitAction, SIGNAL(toggled(bool)),
			that, SLOT(setZoomToFit(bool)) );
		mActionCollection->addAction("view_zoom_to_fit", mZoomToFitAction);

		KAction* actualSizeAction = KStandardAction::actualSize(that, SLOT(zoomActualSize()), mActionCollection);
		actualSizeAction->setIcon(KIcon("zoom-original"));
		actualSizeAction->setIconText(i18nc("@action:button Zoom to original size, shown in status bar, keep it short please", "100%"));
		KStandardAction::zoomIn(that, SLOT(zoomIn()), mActionCollection);
		KStandardAction::zoomOut(that, SLOT(zoomOut()), mActionCollection);

		mZoomWidget->setActions(mZoomToFitAction, actualSizeAction);
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


	void disableZoomToFit() {
		// We can't disable zoom to fit by calling
		// mZoomToFitAction->setChecked(false) directly because it would trigger
		// the action slot, which would set zoom to 100%.
		// If zoomToFit is on and the image is at 33%, pressing zoom in should
		// show the image at 66%, not 200%.
		if (!mAdapter->zoomToFit()) {
			return;
		}
		mAdapter->setZoomToFit(false);
		SignalBlocker blocker(mZoomToFitAction);
		mZoomToFitAction->setChecked(false);
	}


	void setZoom(qreal zoom, const QPoint& center = QPoint(-1, -1)) {
		disableZoomToFit();
		zoom = qBound(computeMinimumZoom(), zoom, MAXIMUM_ZOOM_VALUE);
		mAdapter->setZoom(zoom, center);
	}


	qreal computeMinimumZoom() const {
		// There is no point zooming out less than zoomToFit, but make sure it does
		// not get too small either
		return qMax(0.001, qMin(mAdapter->computeZoomToFit(), 1.));
	}


	void updateZoomSnapValues() {
		qreal min = computeMinimumZoom();
		mZoomWidget->setZoomRange(min, MAXIMUM_ZOOM_VALUE);

		mZoomSnapValues.clear();
		for (qreal zoom = 1/min; zoom > 1. ; zoom -= 1.) {
			mZoomSnapValues << 1/zoom;
		}
		for (qreal zoom = 1; zoom <= MAXIMUM_ZOOM_VALUE ; zoom += 0.5) {
			mZoomSnapValues << zoom;
		}
		mZoomSnapValues
			<< mAdapter->computeZoomToFitWidth()
			<< mAdapter->computeZoomToFitHeight();
		qSort(mZoomSnapValues);
	}
};


DocumentView::DocumentView(QWidget* parent, KActionCollection* actionCollection)
: QWidget(parent)
, d(new DocumentViewPrivate) {
	d->that = this;
	d->mActionCollection = actionCollection;
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	d->mAdapter = 0;
	d->mAdapterCreator = 0;
	d->setupZoomWidget();
	d->setupZoomActions();
	d->createAdapter(createEmptyViewAdapter);
}


DocumentView::~DocumentView() {
	delete d;
}


AbstractDocumentViewAdapter* DocumentView::adapter() const {
	return d->mAdapter;
}


ZoomWidget* DocumentView::zoomWidget() const {
	return d->mZoomWidget;
}

void DocumentView::createAdapterForDocument() {
	AdapterCreator creator = 0;
	AbstractDocumentViewAdapter* adapter = 0;
	switch (d->mDocument->kind()) {
	case MimeTypeUtils::KIND_RASTER_IMAGE:
		creator = createImageViewAdapter;
		break;
	case MimeTypeUtils::KIND_SVG_IMAGE:
		creator = createSvgViewAdapter;
		break;
	default:
		creator = createEmptyViewAdapter;
		adapter = creator(this);
		static_cast<EmptyViewAdapter*>(adapter)->setErrorMessage(i18n("Gwenview does not know how to display this kind of document"));
	}
	Q_ASSERT(creator);

	d->createAdapter(creator, adapter);
	Q_ASSERT(d->mAdapter);
}


void DocumentView::openUrl(const KUrl& url) {
	d->mDocument = DocumentFactory::instance()->load(url);

	while (d->mDocument->loadingState() < Document::KindDetermined) {
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	if (d->mDocument->loadingState() == Document::LoadingFailed) {
		slotLoadingFailed();
		return;
	}
	createAdapterForDocument();

	connect(d->mDocument.data(), SIGNAL(loaded(const KUrl&)),
		SLOT(slotLoaded()) );
	connect(d->mDocument.data(), SIGNAL(loadingFailed(const KUrl&)),
		SLOT(slotLoadingFailed()) );
	d->mAdapter->setDocument(d->mDocument);
	d->updateCaption();
}


void DocumentView::reset() {
	d->createAdapter(createEmptyViewAdapter);
}


bool DocumentView::isEmpty() const {
	//FIXME: Introduce an AbstractDocumentViewAdapter::isEmpty() method
	//instead?
	return d->mAdapterCreator == createEmptyViewAdapter;
}


void DocumentView::slotLoaded() {
	d->updateCaption();
	d->updateZoomSnapValues();
	emit completed();
}


void DocumentView::slotLoadingFailed() {
	AbstractDocumentViewAdapter* adapter = createEmptyViewAdapter(this);
	// FIXME: Get error message from document
	QString message = i18n("Could not load this document");
	static_cast<EmptyViewAdapter*>(adapter)->setErrorMessage(message);
	d->createAdapter(createEmptyViewAdapter, adapter);
	emit completed();
}


void DocumentView::setZoomToFit(bool on) {
	d->mAdapter->setZoomToFit(on);
	if (!on) {
		d->mAdapter->setZoom(1.);
	}
}


void DocumentView::zoomActualSize() {
	d->disableZoomToFit();
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
	d->mZoomWidget->setZoom(zoom);
	d->updateCaption();
}


void DocumentView::slotZoomWidgetChanged(qreal zoom) {
	d->disableZoomToFit();
	d->setZoom(zoom);
}


bool DocumentView::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::MouseButtonPress) {
		// Middle click => toggle zoom to fit
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::MidButton) {
			d->mZoomToFitAction->trigger();
			return true;
		}
	} else if (event->type() == QEvent::Resize) {
		d->updateZoomSnapValues();
	}

	return false;
}


} // namespace
