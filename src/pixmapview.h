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
#ifndef PIXMAPWIDGET_H
#define PIXMAPWIDGET_H

// Qt includes
#include <qmap.h>
#include <qwidgetstack.h>

class QLabel;
class QMouseEvent;
class QPoint;
class QPopupMenu;
class QTimer;
class QWheelEvent;

class KAction;
class KActionCollection;
class KConfig;
class KToggleAction;

class FitPixmapView;
class GVPixmap;
class GVPixmapViewBase;
class ScrollPixmapView;


class PixmapView : public QWidgetStack {
Q_OBJECT
public:
	enum WheelBehaviour { None, Browse, Scroll, Zoom };
	typedef QMap<ButtonState,WheelBehaviour> WheelBehaviours;
	
	PixmapView(QWidget* parent,GVPixmap*,KActionCollection*);
	~PixmapView();

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;
	void installRBPopup(QPopupMenu*);
	void plugActionsToAccel(KAccel*);

	// Properties
	FitPixmapView* fitPixmapView() const { return mFitPixmapView; }
	ScrollPixmapView* scrollPixmapView() const { return mScrollPixmapView; }
	KToggleAction* autoZoom() const { return mAutoZoom; }
	KAction* zoomIn() const { return mZoomIn; }
	KAction* zoomOut() const { return mZoomOut; }
	KAction* resetZoom() const { return mResetZoom; }
	KToggleAction* lockZoom() const { return mLockZoom; }
	double zoom() const;
	bool showPathInFullScreen() const { return mShowPathInFullScreen; }
	void setShowPathInFullScreen(bool);
	WheelBehaviours& wheelBehaviours() { return mWheelBehaviours; }

	
public slots:
	// File operations
	void showFileProperties();
	void openWithEditor();
	void renameFile();
	void copyFile();
	void moveFile();
	void deleteFile();

	void setFullScreen(bool);

signals:
	void selectPrevious();
	void selectNext();
	void zoomChanged(double);

protected:
	bool eventFilter(QObject*,QEvent*);

private:
	ScrollPixmapView* mScrollPixmapView;
	FitPixmapView* mFitPixmapView;
	GVPixmap* mGVPixmap;
	QLabel* mPathLabel;
	QTimer* mAutoHideTimer;
	bool mShowPathInFullScreen;
	WheelBehaviours mWheelBehaviours;
	bool mFullScreen;
	bool mOperaLikePrevious; // Flag to avoid showing the popup menu on Opera like previous

	// Our actions
	KToggleAction* mAutoZoom;
	KAction* mZoomIn;
	KAction* mZoomOut;
	KAction* mResetZoom;
	KToggleAction* mLockZoom;

	KActionCollection* mActionCollection;

	void updatePathLabel();
	GVPixmapViewBase* currentView() const;
	bool mouseMoveEventFilter(QObject*,QMouseEvent*);
	bool mouseReleaseEventFilter(QObject*,QMouseEvent*);
	bool wheelEventFilter(QObject*,QWheelEvent*);

private slots:
	void updateZoomActions();
	void slotAutoZoom();
	void slotUpdateView();
	void hideCursor();
	void openContextMenu(const QPoint&);
};


#endif
