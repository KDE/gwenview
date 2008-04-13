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

// Qt
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QSlider>
#include <QTimer>
#include <QToolButton>

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
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "../lib/imageformats/imageformats.h"
#include "../lib/widgetfloater.h"
#include "gvbrowserextension.h"


//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {

const qreal REAL_DELTA = 0.001;

const qreal ZOOM_MIN = 0.1;
const qreal ZOOM_MAX = 16.;


int sliderValueForZoom(qreal zoom) {
	if (zoom >= 1.) {
		return int( 100 * (zoom - 1.) / (ZOOM_MAX - 1) );
	} else {
		return int( 100 *
			(zoom - ZOOM_MIN) / (1 - ZOOM_MIN)
			) - 100;
	}
}


qreal zoomForSliderValue(int sliderValue) {
	if (sliderValue >= 0) {
		return (ZOOM_MAX - 1) * (sliderValue / 100.) + 1.;
	} else {
		return
			(sliderValue + 100) // [0, 100]
			/ 100.              // [0., 1.]
			* (1 - ZOOM_MIN)    // [0., 1 - ZOOM_MIN]
			+ ZOOM_MIN          // [ZOOM_MIN, 1]
			;
	}
}


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList& args)
: ImageViewPart(parent)
{
	mGwenviewHost = args.contains("gwenviewHost");

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

	updateZoomSnapValues();

	mZoomToFitAction = new KAction(actionCollection());
	mZoomToFitAction->setCheckable(true);
	mZoomToFitAction->setChecked(mView->zoomToFit());
	mZoomToFitAction->setText(i18n("Zoom to Fit"));
	mZoomToFitAction->setIcon(KIcon("zoom-fit-best"));
	connect(mZoomToFitAction, SIGNAL(toggled(bool)), SLOT(setZoomToFit(bool)) );
	actionCollection()->addAction("view_zoom_to_fit", mZoomToFitAction);

	KAction* action = KStandardAction::actualSize(this, SLOT(zoomActualSize()), actionCollection());
	action->setIcon(KIcon("zoom-original"));
	KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
	KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());

	if (!mGwenviewHost) {
		Gwenview::ImageFormats::registerPlugins();
		addPartSpecificActions();
	}

	createStatusBarWidget();
	createErrorLabel();

	mStatusBarExtension = new KParts::StatusBarExtension(this);
	QTimer::singleShot(0, this, SLOT(initStatusBarExtension()) );

	setXMLFile("gvpart/gvpart.rc");

	loadConfig();
}


void GVPart::updateZoomSnapValues() {
	mZoomSnapValues.clear();
	for (qreal zoom = 1/ZOOM_MIN; zoom > 1. ; zoom -= 1.) {
		mZoomSnapValues << 1/zoom;
	}
	for (qreal zoom = 1; zoom <= ZOOM_MAX ; zoom += 0.5) {
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
	mStatusBarWidgetContainer = new QWidget;

	QWidget* container = new QFrame;
	container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	container->setObjectName("zoomStatusBarWidget");
	QHBoxLayout* layout = new QHBoxLayout(mStatusBarWidgetContainer);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addStretch();
	layout->addWidget(container);

	QToolButton* zoomToFitButton = new QToolButton;
	zoomToFitButton->setDefaultAction(actionCollection()->action("view_zoom_to_fit"));
	zoomToFitButton->setObjectName("zoomToFitButton");

	QToolButton* actualSizeButton = new QToolButton;
	actualSizeButton->setDefaultAction(actionCollection()->action("view_actual_size"));
	actualSizeButton->setObjectName("actualSizeButton");

	mZoomLabel = new QLabel;
	mZoomLabel->setObjectName("zoomLabel");
	mZoomLabel->setFixedWidth(mZoomLabel->fontMetrics().width(" 1000% "));
	mZoomLabel->setAlignment(Qt::AlignCenter);

	mZoomSlider = new QSlider;
	mZoomSlider->setObjectName("zoomSlider");
	mZoomSlider->setOrientation(Qt::Horizontal);
	mZoomSlider->setRange(sliderValueForZoom(ZOOM_MIN), sliderValueForZoom(ZOOM_MAX));
	mZoomSlider->setMinimumWidth(200);
	connect(mZoomSlider, SIGNAL(valueChanged(int)), SLOT(applyZoomSliderValue()) );

	layout = new QHBoxLayout(container);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(zoomToFitButton);
	layout->addWidget(actualSizeButton);
	layout->addWidget(mZoomLabel);
	layout->addWidget(mZoomSlider);
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
	if (mStatusBarWidgetContainer->parent()) {
		// Don't show mStatusBarWidgetContainer until it has been embedded in
		// the statusbar by GVPart::initStatusBarExtension()
		mStatusBarWidgetContainer->show();
	}
	mDocument = DocumentFactory::instance()->load(url);
	mView->setDocument(mDocument);
	updateCaption();
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

	mStatusBarWidgetContainer->hide();
}


void GVPart::slotLoaded() {
	emit completed();
	if (mView->zoomToFit()) {
		resizeRequested(mDocument->size());
	}
}


void GVPart::applyZoomSliderValue() {
	disableZoomToFit();
	qreal zoom = zoomForSliderValue(mZoomSlider->value());
	setZoom(zoom);
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
	emit setWindowCaption(caption);
}


void GVPart::slotZoomChanged() {
	int intZoom = int(mView->zoom() * 100);
	mZoomLabel->setText(QString("%1%").arg(intZoom));
	mZoomSlider->blockSignals(true);
	mZoomSlider->setValue(sliderValueForZoom(mView->zoom()));
	mZoomSlider->blockSignals(false);
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
	zoom = qBound(ZOOM_MIN, zoom, ZOOM_MAX);

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

	KIO::Job* job = KIO::file_copy(srcUrl, dstUrl);
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
