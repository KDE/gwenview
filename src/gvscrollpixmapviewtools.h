// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
#ifndef GVSCROLLPIXMAPVIEWTOOLS_H
#define GVSCROLLPIXMAPVIEWTOOLS_H   

// Qt
#include <qcursor.h>

// Local
#include "gvscrollpixmapview.h"


class GVScrollPixmapView::ToolBase {
protected:
	GVScrollPixmapView* mView;

public:
	ToolBase(GVScrollPixmapView* view);
	virtual ~ToolBase();
	virtual void mouseMoveEvent(QMouseEvent*);
	
	virtual void leftButtonPressEvent(QMouseEvent*);
	virtual void leftButtonReleaseEvent(QMouseEvent*);
	
	virtual void midButtonReleaseEvent(QMouseEvent*);

	virtual void rightButtonPressEvent(QMouseEvent* event);
	virtual void rightButtonReleaseEvent(QMouseEvent*);

	virtual void wheelEvent(QWheelEvent* event);

	virtual void updateCursor();
};


class GVScrollPixmapView::ZoomTool : public GVScrollPixmapView::ToolBase {
private:
	QCursor mZoomCursor;

	void zoomTo(const QPoint& pos, bool in); 

public:
	ZoomTool(GVScrollPixmapView* view);
	void leftButtonReleaseEvent(QMouseEvent* event);

	void wheelEvent(QWheelEvent* event);
	void rightButtonPressEvent(QMouseEvent*);
	void rightButtonReleaseEvent(QMouseEvent* event);

	void updateCursor();
};


class GVScrollPixmapView::ScrollTool : public GVScrollPixmapView::ToolBase {
	int mScrollStartX,mScrollStartY;
	bool mDragStarted;

protected:
	QCursor mDragCursor,mDraggingCursor;

public:
	ScrollTool(GVScrollPixmapView* view);
	void leftButtonPressEvent(QMouseEvent* event); 
	void mouseMoveEvent(QMouseEvent* event);
	void leftButtonReleaseEvent(QMouseEvent*);
	void wheelEvent(QWheelEvent* event);
	
	void updateCursor(); 
};


#endif /* GVSCROLLPIXMAPVIEWTOOLS_H */
