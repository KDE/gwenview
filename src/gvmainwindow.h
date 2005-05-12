// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <qptrlist.h>

// KDE
#include <kmainwindow.h>
#include <kurl.h>

// Local
#include "gvimageutils/orientation.h"

#include "config.h"
#ifdef GV_HAVE_KIPI
#include <libkipi/pluginloader.h>
#endif

class QLabel;
class QWidgetStack;

class KAction;
class KDockArea;
class KDockWidget;
class KHistoryCombo;
class KRadioAction;
class KToggleAction;
class KToolBarPopupAction;
class KFileItem;
class KURLCompletion;

class GVDirView;
class GVFileViewStack;
class GVDocument;
class GVHistory;
class GVScrollPixmapView;
class GVSlideShow;
class GVMetaEdit;


class GVMainWindow : public KMainWindow {
Q_OBJECT
public:
	GVMainWindow();

	GVFileViewStack* fileViewStack() const { return mFileViewStack; }
	GVScrollPixmapView* pixmapView() const { return mPixmapView; }
	bool showBusyPtrInFullScreen() const { return mShowBusyPtrInFullScreen; }
	bool showAutoDeleteThumbnailCache() const { return mAutoDeleteThumbnailCache; }
	GVDocument* document() const { return mDocument; }
#ifdef GV_HAVE_KIPI
	KIPI::PluginLoader* pluginLoader() const { return mPluginLoader; }
#endif

	void setShowMenuBarInFullScreen(bool);
	void setShowToolBarInFullScreen(bool);
	void setShowStatusBarInFullScreen(bool);
	void setShowBusyPtrInFullScreen(bool);
	void setAutoDeleteThumbnailCache(bool);

protected:
	bool queryClose();
	virtual void saveProperties( KConfig* );
	virtual void readProperties( KConfig* );

private:
	QWidgetStack* mCentralStack;
	QWidget* mViewModeWidget;
	KDockArea* mDockArea;
	KDockWidget* mFolderDock;
	KDockWidget* mFileDock;
	KDockWidget* mPixmapDock;
	KDockWidget* mMetaDock;
	QLabel* mSBDirLabel;
	QLabel* mSBDetailLabel;

	GVFileViewStack* mFileViewStack;
	GVDirView* mDirView;
	GVScrollPixmapView* mPixmapView;
	GVMetaEdit *mMetaEdit;

	GVDocument* mDocument;
	GVHistory* mGVHistory;
	GVSlideShow* mSlideShow;

	KAction* mRenameFile;
	KAction* mCopyFiles;
	KAction* mMoveFiles;
	KAction* mDeleteFiles;
	KAction* mShowConfigDialog;
	KAction* mShowKeyDialog;
	KToggleAction* mToggleFullScreen;
	KAction* mReload;
	KAction* mOpenHomeDir;
	KToolBarPopupAction* mGoUp;
	KAction* mShowFileProperties;
	KAction* mStartSlideShow;
	KAction* mRotateLeft;
	KAction* mRotateRight;
	KAction* mMirror;
	KAction* mFlip;
	KAction* mSaveFile;
	KAction* mSaveFileAs;
	KAction* mFilePrint;
	KAction* mResetDockWidgets;
	bool	 mLoadingCursor;
	bool	 mAutoDeleteThumbnailCache;
	KToggleAction* mToggleBrowse;

	KHistoryCombo* mURLEdit;
	KURLCompletion* mURLEditCompletion;
	QPtrList<KAction> mWindowListActions;

	bool mShowBusyPtrInFullScreen;

#ifdef GV_HAVE_KIPI
	KIPI::PluginLoader* mPluginLoader;
#endif

	void hideToolBars();
	void showToolBars();
	void createWidgets();
	void createActions();
	void createLocationToolBar();
	void updateLocationURL();
	void createConnections();

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;

private slots:
	void openURL(const KURL&);
	void goUp();
	void goUpTo(int);

	void openHomeDir();
	void renameFile();
	void copyFiles();
	void moveFiles();
	void deleteFiles();
	void showFileProperties();
	void showFileDialog();
	void printFile();  /** print the actual file */
	void clearLocationLabel();
	void activateLocationLabel();

	void toggleFullScreen();
	void showConfigDialog();
	void showExternalToolDialog();
	void showKeyDialog();
	void showToolBarDialog();
	void applyMainWindowSettings();
	void slotImageLoading();
	void slotImageLoaded();
	void startSlideShow();
	void slotDirRenamed(const KURL& oldURL, const KURL& newURL);
	void slotDirURLChanged(const KURL&);
	void modifyImage(GVImageUtils::Orientation);
	void rotateLeft();
	void rotateRight();
	void mirror();
	void flip();
	void resetDockWidgets();

	void slotToggleCentralStack();
	/**
	 * Update status bar and caption
	 */
	void updateStatusInfo();

	/**
	 * Enable or disable image actions
	 */
	void updateImageActions();

	/**
	 * Update only caption, allows setting file info
	 * when folder info is not available yet
	 */
	void updateFileInfo();

	void slotShownFileItemRefreshed(const KFileItem* item);

	/**
	 * Allow quitting full screen mode by pressing Escape key.
	 */
	void escapePressed();

	/**
	 * Address bar related
	 */
	void slotGo();

	void updateWindowActions();

	void loadPlugins();

	// Helper function for updateWindowActions()
	void createHideShowAction(KDockWidget* dock);

	void slotReplug();
};


#endif
