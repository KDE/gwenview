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


#ifndef SCROLLPIXMAPVIEW_H
#define SCROLLPIXMAPVIEW_H

// Qt includes
#include <qpixmap.h>
#include <qscrollview.h>

// Our includes
#include <pixmapviewinterface.h>

class QPainter;
class QPopupMenu;
class QMouseEvent;
class QWheelEvent;

class KAccel;
class KAction;
class KConfig;
class KToggleAction;

class GVPixmap;

class ScrollPixmapView : public QScrollView, public PixmapViewInterface {
Q_OBJECT
public:
	ScrollPixmapView(QWidget* parent,GVPixmap*,bool);
	void enableView(bool);
	void readConfig(KConfig* config, const QString& group);
	void writeConfig(KConfig* config, const QString& group) const;

// Properties
	double zoom() const { return mZoom; }
	void setZoom(double zoom);
	double zoomStep() const { return mZoomStep; }
	void setZoomStep(double zoom);
	void setFullScreen(bool);

public slots:
	void updateView();
	void slotZoomIn();
	void slotZoomOut();
	void slotResetZoom();
	void setLockZoom(bool);

signals:
	void zoomChanged(double);

private:
	GVPixmap* mGVPixmap;
	QPopupMenu* mPopupMenu;

// Offset to center images
	int mXOffset,mYOffset;

// Zoom info
	double mZoom;
	double mZoomStep;
	bool mLockZoom;

// Drag info
	bool mDragStarted; // Indicates that the user is scrolling the image by dragging it
	int mScrollStartX,mScrollStartY;

	void updateImageOffset();
	void updateContentSize();

protected:
// Overloaded methods
	void viewportMousePressEvent(QMouseEvent*);
	void viewportMouseMoveEvent(QMouseEvent*);
	void viewportMouseReleaseEvent(QMouseEvent*);
	void resizeEvent(QResizeEvent* event);
	void drawContents(QPainter* p,int clipx,int clipy,int clipw,int cliph);
};


#endif
