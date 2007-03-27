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
#ifndef GVPART_H
#define GVPART_H

// Qt
#include <QAbstractScrollArea>

// Local
#include "../lib/imageviewpart.h"

class KAboutData;
class KAction;

namespace Gwenview {

class ImageView : public QAbstractScrollArea {
	Q_OBJECT
public:
	ImageView::ImageView(QWidget* parent);

	void setImage(const QImage& image);

	void setZoom(qreal zoom);

	qreal zoom() const;

	bool zoomToFit() const;

public Q_SLOTS:
	void setZoomToFit(bool on);

protected:
	virtual void paintEvent(QPaintEvent*);

	virtual void resizeEvent(QResizeEvent*);

private:
	void updateScrollBars();
	qreal computeZoomToFit() const;
	QImage mImage;
	qreal mZoom;
	bool mZoomToFit;
};

class GVPart : public ImageViewPart {
	Q_OBJECT
public:
	GVPart(QWidget* parentWidget, QObject* parent, const QStringList&);

	static KAboutData* createAboutData();

	virtual Document* document();

protected:
	virtual bool openFile();

private Q_SLOTS:
	void zoomActualSize();
	void zoomIn();
	void zoomOut();

private:
	ImageView* mView;
	Document* mDocument;
	KAction* mZoomToFitAction;
};

} // namespace


#endif /* GVPART_H */
