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
#ifndef FILEVIEWCONTROLLER_H
#define FILEVIEWCONTROLLER_H

// Qt 
#include <qdir.h>
#include <qobject.h>
#include <qslider.h>

// KDE
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio/job.h>
#include <kurl.h>

#include "libgwenview_export.h"
class QIconViewItem;
class QListViewItem;
class QPopupMenu;

class KAccel;
class KAction;
class KActionCollection;
class KConfig;
class KListView;
class KRadioAction;
class KToggleAction;

namespace Gwenview {
class FileViewBase;
class FileDetailView;
class FileThumbnailView;
class ImageLoader;


class DirLister;

class LIBGWENVIEW_EXPORT FileViewController : public QObject {
Q_OBJECT

public:
	enum Mode { FILE_LIST, THUMBNAIL};
	enum FilterMode { ALL, IMAGES_ONLY, VIDEOS_ONLY, CUSTOM };

	FileViewController(QWidget* parent,KActionCollection*);
	~FileViewController();

	// Properties
	void setMode(Mode);
	QWidget* widget() const;
	
	QString fileName() const;
	KURL url() const;
	KURL dirURL() const;
	uint fileCount() const;
	int shownFilePosition() const;
	
	uint selectionSize() const;

	FileViewBase* currentFileView() const;
	FileThumbnailView* fileThumbnailView() const { return mFileThumbnailView; }
	
	KAction* selectFirst() const { return mSelectFirst; }
	KAction* selectLast() const { return mSelectLast; }
	KAction* selectPrevious() const { return mSelectPrevious; }
	KAction* selectNext() const { return mSelectNext; }
	KAction* selectPreviousDir() const { return mSelectPreviousDir; }
	KAction* selectNextDir() const { return mSelectNextDir; }
	KAction* selectFirstSubDir() const { return mSelectFirstSubDir; }
	KRadioAction* listMode() const { return mListMode; }
	KRadioAction* sideThumbnailMode() const { return mSideThumbnailMode; }
	KRadioAction* bottomThumbnailMode() const { return mBottomThumbnailMode; }
	KToggleAction* showDotFiles() const { return mShowDotFiles; }

	KURL::List selectedURLs() const;
	KURL::List selectedImageURLs() const;
	/**
	 * If set to true, no error messages will be displayed.
	 */
	void setSilentMode( bool silent );
	/**
	 * Returns true if there was an error since last URL had been opened.
	 */
	bool lastURLError() const;
	/**
	 * Tries to open again the active URL. Useful for showing error messages
	 * initially supressed by silent mode.
	 */
	void retryURL();

	void refreshItems( const KURL::List& urls ); // used by a workaround in KIPIInterface
	
public slots:
	void setDirURL(const KURL&);
	void setFileNameToSelect(const QString&);

	void slotSelectFirst();
	void slotSelectLast();
	void slotSelectPrevious();
	void slotSelectNext();
	void slotSelectPreviousDir();
	void slotSelectNextDir();
	void slotSelectFirstSubDir();

	void updateThumbnail(const KURL&);

	void openParentDir();
	void showFileProperties();
	void deleteFiles();
	void renameFile();
	void copyFiles();
	void moveFiles();
	void linkFiles();
	void updateFromSettings();


signals:
	void urlChanged(const KURL&);
	/**
	 * Used by DirPart to tell Konqueror to change directory
	 */
	void directoryChanged(const KURL&);

	void selectionChanged();
	void completed();
	void canceled();
	void imageDoubleClicked();
	void shownFileItemRefreshed(const KFileItem*);
	void sortingChanged();

protected slots:
	virtual void openContextMenu(const QPoint& pos, bool onItem);

private slots:
	void delayedDirListerCompleted();
	
	// Used to enter directories
	void slotViewExecuted();

	// Used to change the current image
	void slotViewClicked();

	void slotViewDoubleClicked();
	
	// These two methods forward the context menu requests from either view to
	// openContextMenu(const QPoint&);
	void openContextMenu(KListView*, QListViewItem*, const QPoint&);
	void openContextMenu(QIconViewItem*,const QPoint&);

	// Get called by the thumbnail mode actions
	void updateViewMode();
	
	// Get called by the thumbnail slider
	void updateThumbnailSize(int);

	void toggleShowDotFiles();
	void setSorting();
	void updateSortMenu(QDir::SortSpec);

	// Dir lister slots
	void dirListerDeleteItem(KFileItem* item);
	void dirListerNewItems(const KFileItemList& items);
	void dirListerRefreshItems(const KFileItemList&);
	void dirListerClear();
	void dirListerStarted();
	void dirListerCanceled();
	void dirListerCompleted();

	void openDropURLMenu(QDropEvent*, KFileItem*);

	void makeDir();
	void slotDirMade(KIO::Job* job);
	void prefetchDone();

	void resetNameFilter();
	void resetFromFilter();
	void resetToFilter();
	void updateDirListerFilter();

private:
	struct Private;
	Private* d;
	Mode mMode;
	FileDetailView* mFileDetailView;
	FileThumbnailView* mFileThumbnailView;
	DirLister* mDirLister;
	KURL mDirURL;
	ImageLoader* mPrefetch;

	// Our actions
	KAction* mSelectFirst;
	KAction* mSelectLast;
	KAction* mSelectPrevious;
	KAction* mSelectNext;
	KAction* mSelectPreviousDir;
	KAction* mSelectNextDir;
	KAction* mSelectFirstSubDir;
	
	KRadioAction* mListMode;
	KRadioAction* mSideThumbnailMode;
	KRadioAction* mBottomThumbnailMode;

	QSlider* mSizeSlider;

	KToggleAction* mShowDotFiles;

	// Temp data used by the dir lister
	bool mThumbnailsNeedUpdate;
	QString mFileNameToSelect;
	enum ChangeDirStatusVals {
		CHANGE_DIR_STATUS_NONE,
		CHANGE_DIR_STATUS_PREV,
		CHANGE_DIR_STATUS_NEXT
	} mChangeDirStatus;

	bool mBrowsing;
	bool mSelecting;
	
	/**
	 * Browse to the given item. Prevents multiple calls using mBrowsing.
	 */
	void browseTo(KFileItem* item);

	void browseToFileNameToSelect();
	void emitURLChanged();
	void updateActions();
	void prefetch( KFileItem* item );
	
	KFileItem* findFirstImage() const;
	KFileItem* findLastImage() const;
	KFileItem* findPreviousImage() const;
	KFileItem* findNextImage() const;
	KFileItem* findItemByFileName(const QString& fileName) const;

	bool eventFilter(QObject*, QEvent*);
};


} // namespace
#endif

