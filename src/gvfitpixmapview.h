/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef FITPIXMAPVIEW_H
#define FITPIXMAPVIEW_H


// Qt includes
#include <qframe.h>
#include <qpixmap.h>

// Our includes
#include "gvpixmapviewbase.h"

class QPainter;
class QResizeEvent;

class KConfig;

class GVPixmap;


class GVFitPixmapView : public QFrame, public GVPixmapViewBase {
Q_OBJECT
public:
	GVFitPixmapView(QWidget* parent,GVPixmap* image,bool);
	void enableView(bool);

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;

// Properties
	double zoom() const { return mZoom; }
	void setFullScreen(bool);

public slots:
	void updateView();

signals:
	void zoomChanged(double);

protected:
	void drawContents(QPainter*);
	void resizeEvent(QResizeEvent*);

private:
	GVPixmap* mGVPixmap;
	QPixmap mZoomedPixmap;
	double mZoom;
	
	void paintPixmap(QPainter*);
	void updateZoomedPixmap();
};



#endif
