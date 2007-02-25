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
#include <kactioncollection.h>
#include <kdebug.h>
#include <kstandardaction.h>
#include <kparts/genericfactory.h>

//Factory Code
typedef KParts::GenericFactory<Gwenview::GVPart> GVPartFactory;
K_EXPORT_COMPONENT_FACTORY( gvpart /*library name*/, GVPartFactory )

namespace Gwenview {


GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList&)
: KParts::ReadOnlyPart(parent) 
, mZoom(1.0)
{
	mScene = new QGraphicsScene(parent);
	mView = new QGraphicsView(parentWidget);
	mView->setScene(mScene);
	setWidget(mView);

	KStandardAction::actualSize(this, SLOT(zoomActualSize()), actionCollection());
	KStandardAction::zoomIn(this, SLOT(zoomIn()), actionCollection());
	KStandardAction::zoomOut(this, SLOT(zoomOut()), actionCollection());
	setXMLFile("gvpart/gvpart.rc");
}

bool GVPart::openFile() {
	QPixmap pix(m_file);
	QList<QGraphicsItem*> items = mScene->items();
	if (!items.isEmpty()) {
		QGraphicsItem* item = items.first();
		mScene->removeItem(item);
		delete item;
	}
	mScene->addPixmap(pix);
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

} // namespace
