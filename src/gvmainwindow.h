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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <kdockwidget.h>
#include <kurl.h>


class KAction;
class KHistoryCombo;
class KToggleAction;
class KToolBar;
class KURLCompletion;

class GVDirView;
class GVFileViewStack;
class GVPixmap;
class GVPixmapViewStack;
class GVSlideShow;
class StatusBarProgress;


class GVMainWindow : public KDockMainWindow {
Q_OBJECT
public:
	GVMainWindow();
	~GVMainWindow();

	GVFileViewStack* fileViewStack() const { return mFileViewStack; }
	GVPixmapViewStack* pixmapViewStack() const { return mPixmapViewStack; }
	GVSlideShow* slideShow() const { return mSlideShow; }
	bool showMenuBarInFullScreen() const { return mShowMenuBarInFullScreen; }
	bool showToolBarInFullScreen() const { return mShowToolBarInFullScreen; }
	bool showStatusBarInFullScreen() const { return mShowStatusBarInFullScreen; }
	bool showAddressBar() const { return mShowAddressBar; }

	void setShowMenuBarInFullScreen(bool);
	void setShowToolBarInFullScreen(bool);
	void setShowStatusBarInFullScreen(bool);
	void setShowAddressBar(bool);
	
public slots:
	void setURL(const KURL&,const QString&);

private:
	KDockWidget* mFolderDock;
	KDockWidget* mFileDock;
	KDockWidget* mPixmapDock;
	StatusBarProgress* mProgress;
	KToolBar* mMainToolBar;
	KToolBar* mAddressToolBar;

	GVFileViewStack* mFileViewStack;
	GVDirView* mDirView;
	GVPixmapViewStack* mPixmapViewStack;

	GVPixmap* mGVPixmap;
	GVSlideShow* mSlideShow;

	KAction* mOpenFile;
	KAction* mRenameFile;
	KAction* mCopyFiles;
	KAction* mMoveFiles;
	KAction* mDeleteFiles;
	KAction* mOpenWithEditor;
	KAction* mShowConfigDialog;
	KAction* mShowKeyDialog;
	KToggleAction* mToggleFullScreen;
	KAction* mStop;
	KAction* mOpenParentDir;
	KAction* mShowFileProperties;
	KToggleAction* mToggleSlideShow;
	
	KHistoryCombo* mURLEdit;
	KURLCompletion* mURLEditCompletion;

	bool mShowAddressBar;
	
	bool mShowMenuBarInFullScreen;
	bool mShowToolBarInFullScreen;
	bool mShowStatusBarInFullScreen;
	
	void createWidgets();
	void createActions();
	void createMenu();
	void createMainToolBar();
	void createAddressToolBar();
	void createConnections();

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;
	
private slots:
	void openParentDir();
	void renameFile();
	void copyFiles();
	void moveFiles();
	void deleteFiles();
	void showFileProperties();
	void openFile();
	void openWithEditor();
	
	void toggleFullScreen();
	void showConfigDialog();
	void showKeyDialog();
	void pixmapLoading();
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

	/**
	 * Address bar related
	 */
	void slotURLEditChanged(const QString &str);
};


#endif
