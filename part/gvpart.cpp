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

// Qt
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kicon.h>
#include <kstandardaction.h>
#include <kparts/genericfactory.h>

// Local
#include "../lib/document.h"

//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {

class ImageItem : public QGraphicsItem {
public:
	QRectF boundingRect() const {
		return mImage.rect();
	}

	void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
		painter->drawImage(0, 0, mImage);
	}

	void setImage(const QImage& image) {
		mImage = image;
		update();
	}

	QImage& image() {
		return mImage;
	}

private:
	QImage mImage;
};


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList&)
: ImageViewPart(parent) 
, mZoom(1.0)
{
	mScene = new QGraphicsScene(parent);
	mView = new QGraphicsView(parentWidget);
	mView->setScene(mScene);
	mItem = new ImageItem();
	mScene->addItem(mItem);
	setWidget(mView);

	mDocument = new Document;

	KAction* action = KStandardAction::actualSize(this, SLOT(zoomActualSize()), actionCollection());
	action->setIcon(KIcon("viewmag1"));
	KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
	KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());
	setXMLFile("gvpart/gvpart.rc");
}

bool GVPart::openFile() {
	mDocument->load(localFilePath());
	mItem->setImage(mDocument->image());
	return true;
}


KAboutData* GVPart::createAboutData() {
	KAboutData* aboutData = new KAboutData( "gvpart", I18N_NOOP("GVPart"),
		"2.0", I18N_NOOP("Image Viewer"),
		KAboutData::License_GPL,
		"Copyright 2007, Aurélien Gâteau <aurelien.gateau@free.fr>");
	return aboutData;
}


void GVPart::zoomActualSize() {
	mZoom = 1.;
	updateZoom();
}


void GVPart::zoomIn() {
	mZoom *= 2;
	updateZoom();
}


void GVPart::zoomOut() {
	mZoom /= 2;
	updateZoom();
}


void GVPart::updateZoom() {
	QMatrix matrix;
	matrix.scale(mZoom, mZoom);
	mView->setMatrix(matrix);
}

Document* GVPart::document() {
	return mDocument;
}
} // namespace
