/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kicon.h>
#include <kstandardaction.h>
#include <kparts/genericfactory.h>

// Local
#include "imageview.h"
#include "../lib/documentfactory.h"

//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList&)
: ImageViewPart(parent) 
{
	mView = new ImageView(parentWidget);
	setWidget(mView);

	mZoomToFitAction = new KAction(actionCollection());
	mZoomToFitAction->setCheckable(true);
	mZoomToFitAction->setChecked(mView->zoomToFit());
	mZoomToFitAction->setText(i18n("Zoom To Fit"));
	mZoomToFitAction->setIcon(KIcon("zoom-best-fit"));
	connect(mZoomToFitAction, SIGNAL(toggled(bool)), mView, SLOT(setZoomToFit(bool)) );
	actionCollection()->addAction("view_zoom_to_fit", mZoomToFitAction);

	KAction* action = KStandardAction::actualSize(this, SLOT(zoomActualSize()), actionCollection());
	action->setIcon(KIcon("viewmag1"));
	KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
	KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());
	setXMLFile("gvpart/gvpart.rc");
}


bool GVPart::openFile() {
	return false;
}


bool GVPart::openUrl(const KUrl& url) {
	if (!url.isValid()) {
		return false;
	}
	setUrl(url);
	mDocument = DocumentFactory::instance()->load(url);
	connect(mDocument.data(), SIGNAL(loaded()), SLOT(setViewImageFromDocument()) );
	connect(mDocument.data(), SIGNAL(imageRectUpdated()), SLOT(setViewImageFromDocument()) );
	if (mDocument->isLoaded()) {
		setViewImageFromDocument();
	}
	return true;
}


void GVPart::setViewImageFromDocument() {
	mView->setImage(mDocument->image());
	emit setWindowCaption( url().prettyUrl() );
	emit completed();
}


KAboutData* GVPart::createAboutData() {
	KAboutData* aboutData = new KAboutData( "gvpart", 0, ki18n("GVPart"),
		"2.0", ki18n("Image Viewer"),
		KAboutData::License_GPL,
		ki18n("Copyright 2007, Aurélien Gâteau <aurelien.gateau@free.fr>"));
	return aboutData;
}


void GVPart::zoomActualSize() {
	mZoomToFitAction->setChecked(false);
	mView->setZoom(1.);
}


void GVPart::zoomIn() {
	mZoomToFitAction->setChecked(false);
	mView->setZoom(mView->zoom() * 2);
}


void GVPart::zoomOut() {
	mZoomToFitAction->setChecked(false);
	mView->setZoom(mView->zoom() / 2);
}


Document::Ptr GVPart::document() {
	return mDocument;
}
} // namespace
