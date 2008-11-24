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
#include <kpropertiesdialog.h>

// Local
#include "../lib/gwenviewconfig.h"
#include "../lib/imageview.h"
#include "../lib/signalblocker.h"
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "../lib/documentview/documentview.h"
#include "../lib/imageformats/imageformats.h"
#include "../lib/statusbartoolbutton.h"
#include "../lib/urlutils.h"
#include "../lib/widgetfloater.h"
#include "gvbrowserextension.h"


//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {

static const qreal REAL_DELTA = 0.001;
static const qreal MAXIMUM_ZOOM_VALUE = 16.;


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList& /*args*/)
: KParts::ReadOnlyPart(parent)
{
	QWidget* box = new QWidget(parentWidget);
	mView = new ImageView(box);
	mDocumentView = new DocumentView(box, actionCollection());
	QVBoxLayout* layout = new QVBoxLayout(box);
	layout->addWidget(mView);
	layout->addWidget(mDocumentView);
	setWidget(box);

	mView->setContextMenuPolicy(Qt::CustomContextMenu);
	mView->viewport()->installEventFilter(this);
	connect(mView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );
	connect(mView, SIGNAL(zoomChanged(qreal)), SLOT(slotZoomChanged()) );

	mZoomToFitAction = new KAction(actionCollection());
	mZoomToFitAction->setCheckable(true);
	mZoomToFitAction->setChecked(mView->zoomToFit());
	mZoomToFitAction->setText(i18n("Zoom to Fit"));
	mZoomToFitAction->setIcon(KIcon("zoom-fit-best"));
	mZoomToFitAction->setIconText(i18nc("@action:button Zoom to fit, shown in status bar, keep it short please", "Fit"));
	connect(mZoomToFitAction, SIGNAL(toggled(bool)), SLOT(setZoomToFit(bool)) );
	actionCollection()->addAction("view_zoom_to_fit", mZoomToFitAction);

	KAction* action = new KAction(actionCollection());
	action->setText(i18nc("@action", "Properties"));
	connect(action, SIGNAL(triggered()), SLOT(showProperties()));
	actionCollection()->addAction("file_show_properties", action);

	Gwenview::ImageFormats::registerPlugins();
	addPartSpecificActions();

	createErrorLabel();

	setXMLFile("gvpart/gvpart.rc");

	loadConfig();
}


qreal GVPart::computeMinimumZoom() const {
	// There is no point zooming out less than zoomToFit, but make sure it does
	// not get too small either
	return qMax(0.001, qMin(mView->computeZoomToFit(), 1.));
}

void GVPart::updateZoomSnapValues() {
	qreal min = computeMinimumZoom();

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


void GVPart::showProperties() {
	KPropertiesDialog::showDialog(url(), mView);
}


bool GVPart::openFile() {
	return false;
}


bool GVPart::openUrl(const KUrl& url) {
	if (!url.isValid()) {
		return false;
	}
	setUrl(url);
	mDocumentView->openUrl(url);

	mErrorWidget->hide();
	mDocument = DocumentFactory::instance()->load(url);
	if (arguments().reload()) {
		mDocument->reload();
	}
	if (!UrlUtils::urlIsFastLocalFile(url)) {
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
}


void GVPart::slotLoaded() {
	emit completed();

	// We don't want to emit completed() again if we receive another
	// downSampledImageReady() or loaded() signal from the current document.
	disconnect(mDocument.data(), 0, this, SLOT(slotLoaded()) );

	updateZoomSnapValues();
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


void GVPart::loadConfig() {
	mView->setAlphaBackgroundMode(GwenviewConfig::alphaBackgroundMode());
	mView->setAlphaBackgroundColor(GwenviewConfig::alphaBackgroundColor());
	mView->setEnlargeSmallerImages(GwenviewConfig::enlargeSmallerImages());
}


inline void addActionToMenu(KMenu* menu, KActionCollection* actionCollection, const char* name) {
	QAction* action = actionCollection->action(name);
	if (action) {
		menu->addAction(action);
	}
}


void GVPart::showContextMenu() {
	KMenu menu(mView);
	addActionToMenu(&menu, actionCollection(), "file_save_as");
	menu.addSeparator();
	addActionToMenu(&menu, actionCollection(), "view_actual_size");
	addActionToMenu(&menu, actionCollection(), "view_zoom_to_fit");
	addActionToMenu(&menu, actionCollection(), "view_zoom_in");
	addActionToMenu(&menu, actionCollection(), "view_zoom_out");
	menu.addSeparator();
	addActionToMenu(&menu, actionCollection(), "file_show_properties");
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
