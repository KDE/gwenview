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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <qptrlist.h>

// KDE
#include <kdockwidget.h>
#include <kurl.h>

class QLabel;

class KAction;
class KHistoryCombo;
class KToggleAction;
class KToolBar;
class KToolBarPopupAction;
class KURLCompletion;

class GVDirView;
class GVFileViewStack;
class GVPixmap;
class GVHistory;
class GVScrollPixmapView;
class GVSlideShow;
class GVMetaEdit;
class StatusBarProgress;


class GVMainWindow : public KDockMainWindow {
Q_OBJECT
public:
	GVMainWindow();

	GVFileViewStack* fileViewStack() const { return mFileViewStack; }
	GVScrollPixmapView* pixmapView() const { return mPixmapView; }
	bool showMenuBarInFullScreen() const { return mShowMenuBarInFullScreen; }
	bool showToolBarInFullScreen() const { return mShowToolBarInFullScreen; }
	bool showStatusBarInFullScreen() const { return mShowStatusBarInFullScreen; }
	bool showBusyPtrInFullScreen() const { return mShowBusyPtrInFullScreen; }
	GVPixmap* gvPixmap() const { return mGVPixmap; }

	void setShowMenuBarInFullScreen(bool);
	void setShowToolBarInFullScreen(bool);
	void setShowStatusBarInFullScreen(bool);
	void setShowBusyPtrInFullScreen(bool);
	
public slots:
	void setURL(const KURL&,const QString&);

protected:
	bool queryClose();

private:
	KDockWidget* mFolderDock;
	KDockWidget* mFileDock;
	KDockWidget* mPixmapDock;
	KDockWidget* mMetaDock;
	StatusBarProgress* mProgress;
	KToolBar* mMainToolBar;
	KToolBar* mLocationToolBar;
	QLabel* mSBDirLabel;
	QLabel* mSBDetailLabel;

	GVFileViewStack* mFileViewStack;
	GVDirView* mDirView;
	GVScrollPixmapView* mPixmapView;
	GVMetaEdit *mMetaEdit;

	GVPixmap* mGVPixmap;
	GVHistory* mGVHistory;
	GVSlideShow* mSlideShow;

	KAction* mOpenFile;
	KAction* mRenameFile;
	KAction* mCopyFiles;
	KAction* mMoveFiles;
	KAction* mDeleteFiles;
	KAction* mShowConfigDialog;
	KAction* mShowKeyDialog;
	KToggleAction* mToggleFullScreen;
	KAction* mStop;
	KAction* mReload;
	KAction* mOpenHomeDir;
	KToolBarPopupAction* mGoUp;
	KAction* mShowFileProperties;
	KToggleAction* mToggleSlideShow;
	KAction* mRotateLeft;
	KAction* mRotateRight;
	KAction* mMirror;
	KAction* mFlip;
	KAction* mSaveFile;
	KAction* mSaveFileAs;
	KAction* mFilePrint;
	KAction* mToggleDirAndFileViews;
	
	KHistoryCombo* mURLEdit;
	KURLCompletion* mURLEditCompletion;
	QPtrList<KAction> mWindowListActions;

	bool mShowMenuBarInFullScreen;
	bool mShowToolBarInFullScreen;
	bool mShowStatusBarInFullScreen;
	bool mShowBusyPtrInFullScreen;

	void hideToolBars();
	void showToolBars();
	void createWidgets();
	void createActions();
	void createLocationToolBar();
	void createConnections();

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;
	
private slots:
	void goUp();
	void goUpTo(int);
	
	void openHomeDir();
	void renameFile();
	void copyFiles();
	void moveFiles();
	void deleteFiles();
	void showFileProperties();
	void openFile();
	void printFile();  /** print the actual file */

	void toggleFullScreen();
	void showConfigDialog();
	void showExternalToolDialog();
	void showKeyDialog();
	void showToolBarDialog();
	void applyMainWindowSettings();
	void pixmapLoading();
	void toggleSlideShow();
	void toggleDirAndFileViews();
	void slotDirRenamed(const KURL& oldURL, const KURL& newURL);

	/**
	 * Update status bar and caption
	 */
	void updateStatusInfo();

	/**
	 * Update only caption, allows setting file info
	 * when folder info is not available yet
	 */
	void updateFileInfo();

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
	void slotGo();
	
	void updateWindowActions();

	// Helper function for updateWindowActions()
	void createHideShowAction(KDockWidget* dock);
};


#endif
