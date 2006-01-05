// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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


#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

// Qt
#include <qmap.h>
#include <qscrollview.h>
#include <qtimer.h>
#include <qvaluelist.h>

// Local
#include "busylevelmanager.h"
#include "imageutils/imageutils.h"
#include "libgwenview_export.h"
class QEvent;
class QLabel;
class QMouseEvent;
class QPainter;
class QTimer;
class QWheelEvent;
class KAction;
class KActionCollection;
class KToggleAction;
typedef QValueList<KAction *> KActionPtrList;

namespace Gwenview {
class CaptionFormatterBase;
class Document;

class LIBGWENVIEW_EXPORT ImageView : public QScrollView {
Q_OBJECT

public:
	class ToolBase;
	class ZoomTool;
	class ScrollTool;
	class EventFilter;
#if __GNUC__ < 3
	friend class ToolBase;
	friend class ZoomTool;
	friend class ScrollTool;
#endif
	friend class EventFilter;

	enum ToolID { SCROLL, ZOOM };
	enum OSDMode { NONE, PATH, COMMENT, PATH_AND_COMMENT, FREE_OUTPUT };
	enum ZoomMode { ZOOM_FIT, ZOOM_FIT_WIDTH, ZOOM_FIT_HEIGHT, ZOOM_FREE };
	typedef QMap<ToolID,ToolBase*> Tools;

	ImageView(QWidget* parent,Document*,KActionCollection*);
	~ImageView();

	// Properties
	double zoom() const; 
	void setZoom(double zoom, int centerX=-1, int centerY=-1);
	bool fullScreen() const; 
	void setFullScreenActions(KActionPtrList);
	void setFullScreen(bool);

    void setOSDFormatter(CaptionFormatterBase*);


public slots:
	void updateFromSettings();

signals:
	void selectPrevious();
	void selectNext();
    void doubleClicked();

    // Emitted whenever an hint should be displayed
    void requestHintDisplay(const QString& hint);

protected:
	virtual void openContextMenu(const QPoint&);
	virtual void contentsDragEnterEvent(QDragEnterEvent*);
	virtual void contentsDropEvent(QDropEvent*);
	virtual void keyPressEvent(QKeyEvent*);
	
private:
	struct Private;
	Private* d;

	struct PendingPaint {
		PendingPaint( bool s, const QRect& r ) : rect( r ), smooth( s ) {};
		PendingPaint() {}; // stupid Qt containers
		QRect rect;
		bool smooth;
	};
	enum Operation { CHECK_OPERATIONS = 0, SMOOTH_PASS = 1 << 0 };

	void addPendingPaint( bool smooth, QRect rect = QRect());
	void addPendingPaintInternal( bool smooth, QRect rect = QRect());
	void performPaint( QPainter* painter, int clipx, int clipy, int clipw, int cliph, bool smooth );
	void limitPaintSize( PendingPaint& paint );
	void fullRepaint();
	void cancelPending();
	void scheduleOperation( Operation operation );
	void checkPendingOperationsInternal();
	void updateBusyLevels();

	void updateZoom(ZoomMode, double value=0, int centerX=-1, int centerY=-1);
	double computeZoom(bool in) const;
	double computeZoomToFit() const;
	double computeZoomToWidth() const;
	double computeZoomToHeight() const;
	
	void updateImageOffset();
	void updateScrollBarMode();
	void updateContentSize();
	void updateFullScreenLabel();
	void updateZoomActions();
	void selectTool(ButtonState, bool force);
	void restartAutoHideTimer();

	void emitRequestHintDisplay();

	// Used by the scroll tool
	void emitSelectPrevious() { emit selectPrevious(); }
	void emitSelectNext() { emit selectNext(); }

	// Used by the zoom tool
	QPoint offset() const;
	bool canZoom(bool in) const;
	KToggleAction* zoomToFit() const;

private slots:
	void slotLoaded();
	void slotModified();
	void slotZoomIn();
	void slotZoomOut();
	void slotResetZoom();
	void slotSelectZoom();
	void setZoomToFit(bool);
	void setZoomToWidth(bool);
	void setZoomToHeight(bool);
	void setLockZoom(bool);
	void increaseGamma();
	void decreaseGamma();
	void increaseBrightness();
	void decreaseBrightness();
	void increaseContrast();
	void decreaseContrast();
	void slotAutoHide();
	void slotImageSizeUpdated();
	void slotImageRectUpdated(const QRect&);
	void checkPendingOperations();
	void loadingStarted();
	void slotBusyLevelChanged(BusyLevel);
	void toggleFullScreenBar();
	
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

} // namespace
#endif

