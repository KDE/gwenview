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
#include <QPainter>
#include <QScrollBar>

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


ImageView::ImageView(QWidget* parent)
: QAbstractScrollArea(parent)
{
	mZoom = 1.;
	mZoomToFit = true;
	setFrameShape(QFrame::NoFrame);
	setViewport(new QWidget());
	horizontalScrollBar()->setSingleStep(16);
	verticalScrollBar()->setSingleStep(16);
}

void ImageView::setImage(const QImage& image) {
	mImage = image;
	updateScrollBars();
	update();
}

void ImageView::paintEvent(QPaintEvent*) {
	QWidget* widget = viewport();

	QImage buffer(
		int(widget->width() / mZoom),
		int(widget->height() / mZoom),
		QImage::Format_ARGB32_Premultiplied);
	{
		if (mImage.hasAlphaChannel()) {
			buffer.fill(0);
		}
		QPainter painter(&buffer);
		int posX, posY;
		if (mZoomToFit) {
			posX = 0;
			posY = 0;
		} else {
			posX = int(horizontalScrollBar()->value() / mZoom);
			posY = int(verticalScrollBar()->value() / mZoom);
		}
		painter.drawImage(0, 0, mImage,
			posX, posY,
			buffer.width(),
			buffer.height());
	}
	buffer = buffer.scaled(widget->size());

	QPainter painter(widget);
	painter.drawImage(0, 0, buffer);
}

void ImageView::resizeEvent(QResizeEvent*) {
	if (mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		updateScrollBars();
	}
}

void ImageView::setZoom(qreal zoom) {
	mZoom = zoom;
	updateScrollBars();
	update();
}

qreal ImageView::zoom() const {
	return mZoom;
}

bool ImageView::zoomToFit() const {
	return mZoomToFit;
}

void ImageView::setZoomToFit(bool on) {
	mZoomToFit = on;
	if (mZoomToFit) {
		setZoom(computeZoomToFit());
	} else {
		setZoom(1.);
	}
}

void ImageView::updateScrollBars() {
	if (mZoomToFit) {
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		return;
	}
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	int max;
	int width = viewport()->width();
	int height = viewport()->height();

	max = qMax(0, int(mImage.width() * mZoom) - width);
	horizontalScrollBar()->setRange(0, max);
	horizontalScrollBar()->setPageStep(width);

	max = qMax(0, int(mImage.height() * mZoom) - height);
	verticalScrollBar()->setRange(0, max);
	verticalScrollBar()->setPageStep(height);
}

qreal ImageView::computeZoomToFit() const {
	int width = viewport()->width();
	int height = viewport()->height();
	qreal zoom = qreal(width) / mImage.width();
	if ( int(mImage.height() * zoom) > height) {
		zoom = qreal(height) / mImage.height();
	}
	return zoom;
}




GVPart::GVPart(QWidget* parentWidget, QObject* parent, const QStringList&)
: ImageViewPart(parent) 
{
	mDocument = new Document;
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
	mDocument->load(localFilePath());
	mView->setImage(mDocument->image());
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


Document* GVPart::document() {
	return mDocument;
}
} // namespace
