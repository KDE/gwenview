/*
Gwenview: an image viewer
Copyright 2007-2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "gvpart.moc"

// stdc++
#include <cmath>

// Qt
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QTimer>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kmenu.h>
#include <kstandardaction.h>
#include <kparts/genericfactory.h>
#include <kparts/statusbarextension.h>

// Local
#include "../lib/gwenviewconfig.h"
#include "../lib/imageview.h"
#include "../lib/scrolltool.h"
#include "../lib/signalblocker.h"
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "../lib/imageformats/imageformats.h"
#include "../lib/statusbartoolbutton.h"
#include "../lib/urlutils.h"
#include "../lib/widgetfloater.h"
#include "../lib/zoomwidget.h"
#include "gvbrowserextension.h"


//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {

// FIXME: ZoomWidget
static const qreal REAL_DELTA = 0.001;

static const qreal MAXIMUM_ZOOM_VALUE = 16.;


static const qreal MAGIC_K = 1.04;
static const qreal MAGIC_OFFSET = 16.;
static const qreal PRECISION = 100.;
inline int sliderValueForZoom(qreal zoom) {
	return int( PRECISION * (log(zoom) / log(MAGIC_K) + MAGIC_OFFSET) );
}
// /FIXME: ZoomWidget


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList& args)
: ImageViewPart(parent)
, mZoomUpdatedBySlider(false)
{
	mGwenviewHost = args.contains("gwenviewHost");
	mStatusBarExtension = 0;
	mStatusBarWidgetContainer = 0;

	mView = new ImageView(parentWidget);
	setWidget(mView);

	mScrollTool = new ScrollTool(mView);
	mView->setCurrentTool(mScrollTool);
	connect(mScrollTool, SIGNAL(previousImageRequested()),
		SIGNAL(previousImageRequested()) );
	connect(mScrollTool, SIGNAL(nextImageRequested()),
		SIGNAL(nextImageRequested()) );
	connect(mScrollTool, SIGNAL(zoomInRequested(const QPoint&)),
		SLOT(zoomIn(const QPoint&)) );
	connect(mScrollTool, SIGNAL(zoomOutRequested(const QPoint&)),
		SLOT(zoomOut(const QPoint&)) );

	mView->setContextMenuPolicy(Qt::CustomContextMenu);
	mView->viewport()->installEventFilter(this);
	connect(mView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );
	connect(mView, SIGNAL(zoomChanged()), SLOT(slotZoomChanged()) );

	mZoomToFitAction = new KAction(actionCollection());
	mZoomToFitAction->setCheckable(true);
	mZoomToFitAction->setChecked(mView->zoomToFit());
	mZoomToFitAction->setText(i18n("Zoom to Fit"));
	mZoomToFitAction->setIcon(KIcon("zoom-fit-best"));
	mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));
	connect(mZoomToFitAction, SIGNAL(toggled(bool)), SLOT(setZoomToFit(bool)) );
	actionCollection()->addAction("view_zoom_to_fit", mZoomToFitAction);

	KAction* action = KStandardAction::actualSize(this, SLOT(zoomActualSize()), actionCollection());
	action->setIcon(KIcon("zoom-original"));
	action->setIconText(i18nc("@action:button Zoom to original size, shown in status bar, keep it short please", "100%"));
	KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
	KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());

	if (!mGwenviewHost) {
		Gwenview::ImageFormats::registerPlugins();
		addPartSpecificActions();
	}

	createErrorLabel();

	if (mGwenviewHost) {
		createStatusBarWidget();
		mStatusBarExtension = new KParts::StatusBarExtension(this);
		QTimer::singleShot(0, this, SLOT(initStatusBarExtension()) );
		setXMLFile("gvpart/gvpart-gwenview.rc");
	} else {
		setXMLFile("gvpart/gvpart.rc");
	}

	loadConfig();
}


qreal GVPart::computeMinimumZoom() const {
	// There is no point zooming out less than zoomToFit, but make sure it does
	// not get too small either
	return qMax(0.001, qMin(mView->computeZoomToFit(), 1.));
}

void GVPart::updateZoomSnapValues() {
	qreal min = computeMinimumZoom();
	if (mStatusBarWidgetContainer) {
		mZoomWidget->slider()->setRange(sliderValueForZoom(min), sliderValueForZoom(MAXIMUM_ZOOM_VALUE));
	}

	mZoomSnapValues.clear();
	for (qreal zoom = 1/min; zoom > 1. ; zoom -= 1.) {
		mZoomSnapValues << 1/zoom;
	}
	for (qreal zoom = 1; zoom <= MAXIMUM_ZOOM_VALUE ; zoom += 0.5) {
		mZoomSnapValues << zoom;
	}
	mZoomSnapValues
		<< mView->computeZoomToFitWidth()
		<< mView->computeZoomToFitHeight();
	qSort(mZoomSnapValues);
}


void GVPart::addPartSpecificActions() {
	KStandardAction::saveAs(this, SLOT(saveAs()), actionCollection());

	new GVBrowserExtension(this);
}


void GVPart::createStatusBarWidget() {
	mZoomWidget = new ZoomWidget;
	mZoomWidget->setActions(
		actionCollection()->action("view_zoom_to_fit"),
		actionCollection()->action("view_actual_size"));

	mStatusBarWidgetContainer = new QWidget;
	QHBoxLayout* layout = new QHBoxLayout(mStatusBarWidgetContainer);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addStretch();
	layout->addWidget(mZoomWidget);

	QSlider* slider = mZoomWidget->slider();
	slider->setSingleStep(int(PRECISION));
	slider->setPageStep(3 * slider->singleStep());
	connect(mZoomWidget, SIGNAL(zoomChanged(qreal)),
		SLOT(slotZoomSliderChanged(qreal)) );

	updateZoomSnapValues();
}


void GVPart::createErrorLabel() {
	QPixmap pix = KIconLoader::global()->loadIcon(
		"dialog-error", KIconLoader::Dialog, KIconLoader::SizeMedium);
	QLabel* pixLabel = new QLabel;
	pixLabel->setPixmap(pix);

	mErrorLabel = new QLabel;

	mErrorWidget = new QFrame;
	mErrorWidget->setObjectName("errorWidget");
	mErrorWidget->setStyleSheet(
		"#errorWidget {"
		"	background-color: palette(window);"
		"	border: 1px solid palette(dark);"
		"	padding: 6px;"
		"}"
		);


	QHBoxLayout* layout = new QHBoxLayout(mErrorWidget);
	layout->setMargin(0);
	layout->addWidget(pixLabel);
	layout->addWidget(mErrorLabel);

	WidgetFloater* floater = new WidgetFloater(mView);
	floater->setAlignment(Qt::AlignCenter);
	floater->setChildWidget(mErrorWidget);

	mErrorWidget->hide();
}


void GVPart::initStatusBarExtension() {
	if (!mDocument || mDocument->loadingState() != Document::LoadingFailed) {
		mStatusBarExtension->addStatusBarItem(mStatusBarWidgetContainer, 1, true);
	}
}


bool GVPart::openFile() {
	return false;
}


bool GVPart::openUrl(const KUrl& url) {
	if (!url.isValid()) {
		return false;
	}
	setUrl(url);
	mErrorWidget->hide();
	if (mStatusBarWidgetContainer && mStatusBarWidgetContainer->parent()) {
		// Don't show mStatusBarWidgetContainer until it has been embedded in
		// the statusbar by GVPart::initStatusBarExtension()
		mStatusBarWidgetContainer->show();
	}
	mDocument = DocumentFactory::instance()->load(url);
	if (!mGwenviewHost && !UrlUtils::urlIsFastLocalFile(url)) {
		// Keep raw data of remote files to avoid downloading them again in
		// saveAs()
		mDocument->setKeepRawData(true);
	}
	mView->setDocument(mDocument);
	updateCaption();
	connect(mDocument.data(), SIGNAL(downSampledImageReady()), SLOT(slotLoaded()) );
	connect(mDocument.data(), SIGNAL(loaded(const KUrl&)), SLOT(slotLoaded()) );
	connect(mDocument.data(), SIGNAL(loadingFailed(const KUrl&)), SLOT(slotLoadingFailed()) );
	if (mDocument->loadingState() == Document::Loaded) {
		slotLoaded();
	} else if (mDocument->loadingState() == Document::LoadingFailed) {
		slotLoadingFailed();
	}
	return true;
}


void GVPart::slotLoadingFailed() {
	mView->setDocument(Document::Ptr());
	emit completed();
	QString msg = i18n("Could not load <filename>%1</filename>.", url().fileName());
	mErrorLabel->setText(msg);
	mErrorWidget->adjustSize();
	mErrorWidget->show();

	if (mStatusBarWidgetContainer) {
		mStatusBarWidgetContainer->hide();
	}
}


void GVPart::slotLoaded() {
	emit completed();
	if (mView->zoomToFit()) {
		resizeRequested(mDocument->size());
	}

	// We don't want to emit completed() again if we receive another
	// downSampledImageReady() or loaded() signal from the current document.
	disconnect(mDocument.data(), 0, this, SLOT(slotLoaded()) );

	updateZoomSnapValues();
}


void GVPart::slotZoomSliderChanged(qreal zoom) {
	disableZoomToFit();

	// Set this flag to prevent slotZoomChanged() from changing the slider
	// value
	mZoomUpdatedBySlider = true;
	setZoom(zoom);
	mZoomUpdatedBySlider = false;
}


KAboutData* GVPart::createAboutData() {
	KAboutData* aboutData = new KAboutData(
		"gvpart",                /* appname */
		"gwenview",              /* catalogName */
		ki18n("Gwenview KPart"), /* programName */
		"2.1");                  /* version */
	aboutData->setShortDescription(ki18n("An Image Viewer"));
	aboutData->setLicense(KAboutData::License_GPL);
	aboutData->setCopyrightStatement(ki18n("Copyright 2000-2008 Aurélien Gâteau"));
	aboutData->addAuthor(
		ki18n("Aurélien Gâteau"),
		ki18n("Main developer"),
		"aurelien.gateau@free.fr");
	return aboutData;
}


void GVPart::updateCaption() {
	QString caption = url().fileName();
	QSize size = mDocument->size();
	if (size.isValid()) {
		int intZoom = qRound(mView->zoom() * 100);
		caption +=
			QString(" - %1x%2 - %3%")
				.arg(size.width())
				.arg(size.height())
				.arg(intZoom);
	}
	emit setWindowCaption(caption);
}


void GVPart::slotZoomChanged() {
	if (mStatusBarWidgetContainer) {
		int intZoom = qRound(mView->zoom() * 100);
		mZoomWidget->label()->setText(QString("%1%").arg(intZoom));

		// Update slider, but only if the change does not come from it.
		if (!mZoomUpdatedBySlider) {
			SignalBlocker blocker(mZoomWidget->slider());
			int value = sliderValueForZoom(mView->zoom());
			mZoomWidget->slider()->setValue(value);
		}
	}
	updateCaption();
}


void GVPart::setZoomToFit(bool on) {
	mView->setZoomToFit(on);
	if (!on) {
		mView->setZoom(1.);
	}
}


void GVPart::zoomActualSize() {
	disableZoomToFit();
	mView->setZoom(1.);
}


void GVPart::zoomIn(const QPoint& center) {
	qreal currentZoom = mView->zoom();
	Q_FOREACH(qreal zoom, mZoomSnapValues) {
		if (zoom > currentZoom + REAL_DELTA) {
			setZoom(zoom, center);
			return;
		}
	}
}


void GVPart::zoomOut(const QPoint& center) {
	qreal currentZoom = mView->zoom();

	QListIterator<qreal> it(mZoomSnapValues);
	it.toBack();
	while (it.hasPrevious()) {
		qreal zoom = it.previous();
		if (zoom < currentZoom - REAL_DELTA) {
			setZoom(zoom, center);
			return;
		}
	}
}


void GVPart::setZoom(qreal zoom, const QPoint& _center) {
	disableZoomToFit();
	QPoint center;
	if (_center == QPoint(-1, -1)) {
		center = QPoint(mView->viewport()->width() / 2, mView->viewport()->height() / 2);
	} else {
		center = _center;
	}
	zoom = qBound(computeMinimumZoom(), zoom, MAXIMUM_ZOOM_VALUE);

	mView->setZoom(zoom, center);
}


void GVPart::disableZoomToFit() {
	// We can't disable zoom to fit by calling
	// mZoomToFitAction->setChecked(false) directly because it would trigger
	// the action slot, which would set zoom to 100%.
	// If zoomToFit is on and the image is at 33%, pressing zoom in should
	// show the image at 66%, not 200%.
	if (!mView->zoomToFit()) {
		return;
	}
	mView->setZoomToFit(false);
	mZoomToFitAction->blockSignals(true);
	mZoomToFitAction->setChecked(false);
	mZoomToFitAction->blockSignals(false);
}


Document::Ptr GVPart::document() {
	return mDocument;
}


ImageView* GVPart::imageView() const {
	return mView;
}


void GVPart::loadConfig() {
	mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
	mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
	mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
	mScrollTool->setMouseWheelBehavior(GwenviewConfig::mouseWheelBehavior());
}


inline void addActionToMenu(KMenu* menu, KActionCollection* actionCollection, const char* name) {
	QAction* action = actionCollection->action(name);
	if (action) {
		menu->addAction(action);
	}
}


void GVPart::showContextMenu() {
	KMenu menu(mView);
	if (!mGwenviewHost) {
		addActionToMenu(&menu, actionCollection(), "file_save_as");
		menu.addSeparator();
	}
	addActionToMenu(&menu, actionCollection(), "view_actual_size");
	addActionToMenu(&menu, actionCollection(), "view_zoom_to_fit");
	addActionToMenu(&menu, actionCollection(), "view_zoom_in");
	addActionToMenu(&menu, actionCollection(), "view_zoom_out");
	menu.exec(QCursor::pos());
}


bool GVPart::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::MouseButtonPress) {
		// Middle click => toggle zoom to fit
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::MidButton) {
			mZoomToFitAction->trigger();
			return true;
		}
	} else if (event->type() == QEvent::Resize) {
		updateZoomSnapValues();
	}

	return false;
}


void GVPart::saveAs() {
	KUrl srcUrl = url();
	KUrl dstUrl = KFileDialog::getSaveUrl(srcUrl.fileName(), QString(), widget());
	if (!dstUrl.isValid()) {
		return;
	}

	KIO::Job* job;
	QByteArray rawData = mDocument->rawData();
	if (rawData.length() > 0) {
		job = KIO::storedPut(rawData, dstUrl, -1);
	} else {
		job = KIO::file_copy(srcUrl, dstUrl);
	}
	connect(job, SIGNAL(result(KJob*)),
		this, SLOT(showJobError(KJob*)) );
}


void GVPart::showJobError(KJob* job) {
	if (job->error() != 0) {
		KIO::JobUiDelegate* ui = static_cast<KIO::Job*>(job)->ui();
		if (!ui) {
			kError() << "Saving failed. job->ui() is null.";
			return;
		}
		ui->setWindow(widget());
		ui->showErrorMessage();
	}
}


} // namespace
