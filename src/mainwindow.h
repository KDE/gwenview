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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <kdockwidget.h>
#include <kurl.h>


class KAction;
class KAccel;
class KToggleAction;

class DirView;
class GVFileViewStack;
class GVPixmap;
class GVSlideShow;
class PixmapView;
class StatusBarProgress;


class MainWindow : public KDockMainWindow {
Q_OBJECT
public:
	MainWindow();
	~MainWindow();

	GVFileViewStack* fileViewStack() const { return mFileViewStack; }
	PixmapView* pixmapView() const { return mPixmapView; }
	GVSlideShow* slideShow() const { return mSlideShow; }
	bool showMenuBarInFullScreen() const { return mShowMenuBarInFullScreen; }
	bool showToolBarInFullScreen() const { return mShowToolBarInFullScreen; }
	bool showStatusBarInFullScreen() const { return mShowStatusBarInFullScreen; }

	void setShowMenuBarInFullScreen(bool);
	void setShowToolBarInFullScreen(bool);
	void setShowStatusBarInFullScreen(bool);
	
public slots:
	void setURL(const KURL&,const QString&);

private:
	KDockWidget* mFolderDock;
	KDockWidget* mFileDock;
	KDockWidget* mPixmapDock;
	StatusBarProgress* mProgress;

	GVFileViewStack* mFileViewStack;
	DirView* mDirView;
	PixmapView* mPixmapView;

	GVPixmap* mGVPixmap;
	GVSlideShow* mSlideShow;

	KAction* mOpenFile;
	KAction* mOpenLocation;
	KAction* mRenameFile;
	KAction* mCopyFile;
	KAction* mMoveFile;
	KAction* mDeleteFile;
	KAction* mOpenWithEditor;
	KAction* mShowConfigDialog;
	KAction* mShowKeyDialog;
	KToggleAction* mToggleFullScreen;
	KAction* mStop;
	KAction* mOpenParentDir;
	KAction* mShowFileProperties;
	KToggleAction* mToggleSlideShow;
	
	KAccel* mAccel;

	bool mShowMenuBarInFullScreen,mShowToolBarInFullScreen,mShowStatusBarInFullScreen;
	
	void createWidgets();
	void createActions();
	void createAccels();
	void createMenu();
	void createToolBar();
	void createPixmapViewPopupMenu();

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;
	
private slots:
	void openFile();
	void openLocation();
	void toggleFullScreen();
	void showConfigDialog();
	void showKeyDialog();
	void pixmapLoading();
	void openWithEditor();
	void openParentDir();
	void showFileProperties();
	void toggleSlideShow();

	/**
	 * Update both folder and file status bar
	 */
	void updateStatusBar();

	/**
	 * Update only file status bar, allows setting file info
	 * when folder info is not available yet
	 */
	void updateFileStatusBar();

	void thumbnailUpdateStarted(int);
	void thumbnailUpdateEnded();
	void thumbnailUpdateProcessedOne();

	/**
	 * Allow quitting full screen mode by pressing Escape key.
	 */
	void escapePressed();
};


#endif
