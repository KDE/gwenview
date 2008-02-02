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
#include <QMouseEvent>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kicon.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kmenu.h>
#include <kstandardaction.h>
#include <kparts/genericfactory.h>

// Local
#include "../lib/gwenviewconfig.h"
#include "../lib/imageview.h"
#include "../lib/scrolltool.h"
#include "../lib/document/document.h"
#include "../lib/document/documentfactory.h"
#include "gvbrowserextension.h"

//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList& args)
: ImageViewPart(parent)
{
	mGwenviewHost = args.contains("gwenviewHost");

	mView = new ImageView(parentWidget);
	setWidget(mView);
	ScrollTool* scrollTool = new ScrollTool(mView);
	mView->setCurrentTool(scrollTool);
	mView->setContextMenuPolicy(Qt::CustomContextMenu);
	mView->viewport()->installEventFilter(this);
	connect(mView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );
	connect(mView, SIGNAL(zoomChanged()), SLOT(updateCaption()) );

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
		addPartSpecificActions();
	}

	setXMLFile("gvpart/gvpart.rc");

	loadConfig();
}


void GVPart::addPartSpecificActions() {
	KStandardAction::saveAs(this, SLOT(saveAs()), actionCollection());

	new GVBrowserExtension(this);
}


bool GVPart::openFile() {
	return false;
}


bool GVPart::openUrl(const KUrl& url) {
	if (!url.isValid()) {
		return false;
	}
	setUrl(url);
	mView->setImage(0);
	mDocument = DocumentFactory::instance()->load(url);
	connect(mDocument.data(), SIGNAL(loaded(const KUrl&)), SIGNAL(completed()) );
	connect(mDocument.data(), SIGNAL(imageRectUpdated(const QRect&)), SLOT(slotImageRectUpdated(const QRect&)) );
	if (mDocument->isLoaded()) {
		setViewImageFromDocument();
		mView->updateImageRect(mDocument->image().rect());
		emit completed();
	}
	return true;
}


void GVPart::setViewImageFromDocument() {
	mView->setImage(&mDocument->image());
	updateCaption();
	if (mView->zoomToFit()) {
		resizeRequested(mDocument->image().size());
	}
}


void GVPart::slotImageRectUpdated(const QRect& rect) {
	if (!mView->image()) {
		setViewImageFromDocument();
	}
	mView->updateImageRect(rect);
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
	int intZoom = int(mView->zoom() * 100);
	QString urlString = url().fileName();
	QString caption = QString("%1 - %2%").arg(urlString).arg(intZoom);
	emit setWindowCaption(caption);
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


void GVPart::zoomIn() {
	disableZoomToFit();
	mView->setZoom(mView->zoom() * 2);
}


void GVPart::zoomOut() {
	disableZoomToFit();
	if (mView->zoom() > 0.01) {
		mView->setZoom(mView->zoom() / 2);
	}
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
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		if (mouseEvent->button() == Qt::MidButton) {
			mZoomToFitAction->trigger();
			return true;
		}
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
