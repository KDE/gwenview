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
#include <kactioncategory.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/documentview/messageviewadapter.h>
#include <lib/documentview/imageviewadapter.h>
#include <lib/documentview/svgviewadapter.h>
#include <lib/documentview/videoviewadapter.h>
#include <lib/kpixmapsequence.h>
#include <lib/pixmapsequencecontroller.h>
#include <lib/mimetypeutils.h>
#include <lib/signalblocker.h>
#include <lib/slideshow.h>
#include <lib/widgetfloater.h>
#include <lib/zoomwidget.h>

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
static const qreal MAXIMUM_ZOOM_VALUE = 16.;


struct DocumentViewPrivate {
	DocumentView* that;
	SlideShow* mSlideShow;
	KActionCollection* mActionCollection;
	ZoomWidget* mZoomWidget;
	KAction* mZoomToFitAction;

	KPixmapSequence mLoadingPixmapSequence;
	PixmapSequenceController mLoadingPixmapSequenceController;

	bool mZoomWidgetVisible;
	AbstractDocumentViewAdapter* mAdapter;
	QList<qreal> mZoomSnapValues;
	Document::Ptr mDocument;
	QLabel* mLoadingIndicator;


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
			if (mZoomWidgetVisible) {
				mZoomWidget->show();
			}
		} else {
			mZoomWidget->hide();
		}
		mAdapter->installEventFilterOnViewWidgets(that);

		updateActions();
	}


	void setupZoomWidget() {
		mZoomWidget = new ZoomWidget;
		QObject::connect(mZoomWidget, SIGNAL(zoomChanged(qreal)),
			that, SLOT(slotZoomWidgetChanged(qreal)) );
	}

	void setupZoomActions() {
		KActionCategory* view=new KActionCategory(i18nc("@title actions category - means actions changing smth in interface","View"), mActionCollection);

		mZoomToFitAction = view->addAction("view_zoom_to_fit");
		mZoomToFitAction->setCheckable(true);
		mZoomToFitAction->setChecked(true);
		mZoomToFitAction->setText(i18n("Zoom to Fit"));
		mZoomToFitAction->setIcon(KIcon("zoom-fit-best"));
		mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));
		QObject::connect(mZoomToFitAction, SIGNAL(toggled(bool)),
			that, SLOT(setZoomToFit(bool)) );

		KAction* actualSizeAction = view->addAction(KStandardAction::ActualSize,that, SLOT(zoomActualSize()));
		actualSizeAction->setIcon(KIcon("zoom-original"));
		actualSizeAction->setIconText(i18nc("@action:button Zoom to original size, shown in status bar, keep it short please", "100%"));
		view->addAction(KStandardAction::ZoomIn,that, SLOT(zoomIn()));
		view->addAction(KStandardAction::ZoomOut,that, SLOT(zoomOut()));

		mZoomWidget->setActions(mZoomToFitAction, actualSizeAction);
	}


	inline void setActionEnabled(const char* name, bool enabled) {
		QAction* action = mActionCollection->action(name);
		if (action) {
			action->setEnabled(enabled);
		} else {
			kWarning() << "Action" << name << "not found";
		}
	}

	void updateActions() {
		const bool enabled = that->isVisible() && mAdapter->canZoom();
		mZoomToFitAction->setEnabled(enabled);
		setActionEnabled("view_actual_size", enabled);
		setActionEnabled("view_zoom_in", enabled);
		setActionEnabled("view_zoom_out", enabled);
	}


	void setupLoadingIndicator() {
		mLoadingIndicator = new QLabel;

		const QString path = KIconLoader::global()->iconPath("process-working", -22);
		mLoadingPixmapSequence.load(path);
		mLoadingIndicator->setFixedSize(mLoadingPixmapSequence.frameSize());
		mLoadingPixmapSequenceController.setPixmapSequence(mLoadingPixmapSequence);
		mLoadingPixmapSequenceController.setInterval(100);
		QObject::connect(&mLoadingPixmapSequenceController, SIGNAL(frameChanged(const QPixmap&)),
			mLoadingIndicator, SLOT(setPixmap(const QPixmap&)));

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
		// We can't uncheck zoom to fit by calling
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
		uncheckZoomToFit();
		zoom = qBound(computeMinimumZoom(), zoom, MAXIMUM_ZOOM_VALUE);
		mAdapter->setZoom(zoom, center);
	}


	qreal computeMinimumZoom() const {
		// There is no point zooming out less than zoomToFit, but make sure it does
		// not get too small either
		return qMax(0.001, qMin(double(mAdapter->computeZoomToFit()), 1.));
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


	void showLoadingIndicator() {
		if (!mLoadingIndicator) {
			setupLoadingIndicator();
		}
		mLoadingPixmapSequenceController.start();
		mLoadingIndicator->show();
		mLoadingIndicator->raise();
	}


	void hideLoadingIndicator() {
		if (!mLoadingIndicator) {
			return;
		}
		mLoadingPixmapSequenceController.stop();
		mLoadingIndicator->hide();
	}
};


DocumentView::DocumentView(QWidget* parent, SlideShow* slideShow, KActionCollection* actionCollection)
: QWidget(parent)
, d(new DocumentViewPrivate) {
	d->that = this;
	d->mSlideShow = slideShow;
	d->mActionCollection = actionCollection;
	d->mLoadingIndicator = 0;
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	d->mAdapter = 0;
	d->mZoomWidgetVisible = true;
	d->setupZoomWidget();
	d->setupZoomActions();
	d->setCurrentAdapter(new MessageViewAdapter(this));
}


DocumentView::~DocumentView() {
	delete d;
}


void DocumentView::setZoomWidgetVisible(bool visible) {
	d->mZoomWidgetVisible = visible;
	if (!visible) {
		d->mZoomWidget->hide();
	}
}


AbstractDocumentViewAdapter* DocumentView::adapter() const {
	return d->mAdapter;
}


ZoomWidget* DocumentView::zoomWidget() const {
	return d->mZoomWidget;
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
		if (d->mSlideShow) {
			connect(adapter, SIGNAL(videoFinished()),
				d->mSlideShow, SLOT(resumeAndGoToNextUrl()));
		}
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
	d->mZoomWidget->setZoom(zoom);
	d->updateCaption();
}


void DocumentView::slotZoomWidgetChanged(qreal zoom) {
	d->uncheckZoomToFit();
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
	} else if (event->type() == QEvent::MouseButtonDblClick) {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->modifiers() == Qt::NoModifier) {
			emit toggleFullScreenRequested();
			return true;
		}
	}

	return false;
}


void DocumentView::showEvent(QShowEvent*) {
	d->updateActions();
}


void DocumentView::hideEvent(QHideEvent*) {
	d->updateActions();
}


} // namespace
