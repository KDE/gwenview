// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau

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


#ifndef GVSCROLLPIXMAPVIEW_H
#define GVSCROLLPIXMAPVIEW_H

// Qt includes
#include <qmap.h>
#include <qscrollview.h>

class QEvent;
class QLabel;
class QMouseEvent;
class QPainter;
class QTimer;
class QWheelEvent;
class KAction;
class KActionCollection;
class KConfig;
class KToggleAction;

class GVPixmap;

class GVScrollPixmapView : public QScrollView {
Q_OBJECT

public:
	class ToolController;
	class ScrollToolController;
	class ZoomToolController;

	friend class ToolController;
	friend class ScrollToolController;
	friend class ZoomToolController;

	enum Tool { None, Browse, Scroll, Zoom };
	typedef QMap<ButtonState,Tool> ButtonStateToolMap;
	typedef QMap<Tool,ToolController*> ToolControllers;

	GVScrollPixmapView(QWidget* parent,GVPixmap*,KActionCollection*);
	~GVScrollPixmapView();
	void readConfig(KConfig* config, const QString& group);
	void writeConfig(KConfig* config, const QString& group) const;

	// Properties
	KToggleAction* autoZoom() const { return mAutoZoom; }
	KAction* zoomIn() const { return mZoomIn; }
	KAction* zoomOut() const { return mZoomOut; }
	KAction* resetZoom() const { return mResetZoom; }
	KToggleAction* lockZoom() const { return mLockZoom; }
	double zoom() const { return mZoom; }
	void setZoom(double zoom, int centerX=-1, int centerY=-1);
	bool fullScreen() const { return mFullScreen; }
	void setFullScreen(bool);
	bool showPathInFullScreen() const { return mShowPathInFullScreen; }
	void setShowPathInFullScreen(bool);
	bool smoothScale() const { return mSmoothScale; }
	void setSmoothScale(bool);
	bool enlargeSmallImages() const { return mEnlargeSmallImages; }
	void setEnlargeSmallImages(bool);
	bool showScrollBars() const { return mShowScrollBars; }
	void setShowScrollBars(bool);
	void setAutoZoomBrowse(bool);

	Tool buttonStateTool(ButtonState bs) const { return mButtonStateToolMap[bs]; }
	void setButtonStateTool(ButtonState,Tool);

	void restartAutoHideTimer();

	// Used by the browse tool controller
	void emitSelectPrevious() { emit selectPrevious(); }
	void emitSelectNext() { emit selectNext(); }

public slots:
	// File operations
	void showFileProperties();
	void renameFile();
	void copyFile();
	void moveFile();
	void deleteFile();

signals:
	void selectPrevious();
	void selectNext();
	void zoomChanged(double);

private:
	GVPixmap* mGVPixmap;
	QTimer* mAutoHideTimer;
	QLabel* mPathLabel;
	
	bool mShowPathInFullScreen;
	bool mSmoothScale;
	bool mEnlargeSmallImages;
	bool mShowScrollBars;
	ButtonStateToolMap mButtonStateToolMap;
	ToolControllers mToolControllers;

	Tool mTool;

	// Offset to center images
	int mXOffset,mYOffset;

	// Zoom info
	double mZoom;

	// Our actions
	KToggleAction* mAutoZoom;
	KAction* mZoomIn;
	KAction* mZoomOut;
	KAction* mResetZoom;
	KToggleAction* mLockZoom;
	KActionCollection* mActionCollection;

	// Object state info
	bool mFullScreen;
	bool mOperaLikePrevious; // Flag to avoid showing the popup menu on Opera like previous
	double mZoomBeforeAuto;
	int mXCenterBeforeAuto, mYCenterBeforeAuto;

	double computeAutoZoom();
	void updateScrollBarMode();
	void updateImageOffset();
	void updateContentSize();
	void openContextMenu(const QPoint&);
	void updatePathLabel();
	void updateZoomActions();
	void selectTool(ButtonState, bool force);

private slots:
	void slotURLChanged();
	void slotModified();
	void slotZoomIn();
	void slotZoomOut();
	void slotResetZoom();
	void setAutoZoom(bool);
	void hideCursor();

	
protected:
	// Overloaded methods
	bool eventFilter(QObject*, QEvent*);
	void viewportMousePressEvent(QMouseEvent*);
	void viewportMouseMoveEvent(QMouseEvent*);
	void viewportMouseReleaseEvent(QMouseEvent*);
	bool viewportKeyEvent(QKeyEvent*); // This one is not inherited, it's called from the eventFilter
	void wheelEvent(QWheelEvent* event);
	void resizeEvent(QResizeEvent* event);
	void drawContents(QPainter* p,int clipx,int clipy,int clipw,int cliph);
};


#endif
