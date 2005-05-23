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
#ifndef GVFILEVIEWSTACK_H
#define GVFILEVIEWSTACK_H

// Qt 
#include <qdir.h>
#include <qobject.h>
#include <qslider.h>
#include <qwidgetstack.h>

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

class GVFileViewBase;
class GVFileDetailView;
class GVFileThumbnailView;


class GVFileViewStackPrivate;

// internal class which allows dynamically turning off visual error reporting
class GVDirLister : public KDirLister {
Q_OBJECT
public:
	GVDirLister() : KDirLister(), mError( false ), mCheck( false ) {}
	virtual bool validURL( const KURL& ) const;
	virtual void handleError( KIO::Job * );
	bool error() const { return mError; }
	void clearError() { mError = false; }
	void setCheck( bool c ) { mCheck = c; }
private:
	mutable bool mError;
	bool mCheck;
};

class LIBGWENVIEW_EXPORT GVFileViewStack : public QWidgetStack {
Q_OBJECT

public:
	enum Mode { FILE_LIST, THUMBNAIL};

	GVFileViewStack(QWidget* parent,KActionCollection*);
	~GVFileViewStack();

	// Config
	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;
	/**
	 * Used by the KParts, equivalent of readConfig(), this sets
	 * some values but just uses the defaults rather than using
	 * KConfig
	 */
	void kpartConfig();

	// Properties
	void setMode(Mode);
	
	QString fileName() const;
	KURL url() const;
	KURL dirURL() const;
	uint fileCount() const;
	
	bool showDirs() const;
	void setShowDirs(bool);
	
	uint selectionSize() const;

	QColor shownColor() const { return mShownColor; }
	void setShownColor(const QColor&);
	
	GVFileViewBase* currentFileView() const;
	GVFileThumbnailView* fileThumbnailView() const { return mFileThumbnailView; }
	
	KAction* selectFirst() const { return mSelectFirst; }
	KAction* selectLast() const { return mSelectLast; }
	KAction* selectPrevious() const { return mSelectPrevious; }
	KAction* selectNext() const { return mSelectNext; }
	KRadioAction* listMode() const { return mListMode; }
	KRadioAction* sideThumbnailMode() const { return mSideThumbnailMode; }
	KRadioAction* bottomThumbnailMode() const { return mBottomThumbnailMode; }
	KToggleAction* showDotFiles() const { return mShowDotFiles; }

	void setFocus();

	KURL::List selectedURLs() const;
	/**
	 * If set to true, no error messages will be displayed.
	 */
	void setSilentMode( bool silent );
	/**
	 * Returns true if there was an error since last URL had been opened.
	 */
	bool lastURLError() const { return mDirLister->error(); }
	/**
	 * Tries to open again the active URL. Useful for showing error messages
	 * initially supressed by silent mode.
	 */
	void retryURL();

	void refreshItems( const KURL::List& urls ); // used by a workaround in GVKIPIInterface
	
public slots:
	void setDirURL(const KURL&);
	void setFileNameToSelect(const QString&);

	void slotSelectFirst();
	void slotSelectLast();
	void slotSelectPrevious();
	void slotSelectNext();

	void updateThumbnail(const KURL&);

	void openParentDir();
	void showFileProperties();
	void deleteFiles();
	void renameFile();
	void copyFiles();
	void moveFiles();


signals:
	void urlChanged(const KURL&);
	/**
	 * Used by GVDirPart to tell Konqueror to change directory
	 */
	void directoryChanged(const KURL&);

	void selectionChanged();
	void completed();
	void canceled();
	void completedURLListing(const KURL&);
	void imageDoubleClicked();
	void shownFileItemRefreshed(const KFileItem*);


private slots:
	void delayedDirListerCompleted();
	
	// Used to enter directories
	void slotViewExecuted();

	// Used to change the current image
	void slotViewClicked();

	void slotViewDoubleClicked();
	
	// Context menu
	void openContextMenu(const QPoint& pos);
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
	
private:
	GVFileViewStackPrivate* d;
	Mode mMode;
	GVFileDetailView* mFileDetailView;
	GVFileThumbnailView* mFileThumbnailView;
	GVDirLister* mDirLister;
	KURL mDirURL;

	// Our actions
	KAction* mSelectFirst;
	KAction* mSelectLast;
	KAction* mSelectPrevious;
	KAction* mSelectNext;
	
	KRadioAction* mListMode;
	KRadioAction* mSideThumbnailMode;
	KRadioAction* mBottomThumbnailMode;

	QSlider* mSizeSlider;

	KToggleAction* mShowDotFiles;

	// configurable settings
	bool mShowDirs;
	QColor mShownColor;

	// Temp data used by the dir lister
	bool mThumbnailsNeedUpdate;
	QString mFileNameToSelect;

	bool mBrowsing;
	bool mSelecting;
	
	/**
	 * Browse to the given item. Prevents multiple calls using mBrowsing.
	 */
	void browseTo(KFileItem* item);

	void browseToFileNameToSelect();
	void emitURLChanged();
	void updateActions();
	void initDirListerFilter();
	
	KFileItem* findFirstImage() const;
	KFileItem* findLastImage() const;
	KFileItem* findPreviousImage() const;
	KFileItem* findNextImage() const;
	KFileItem* findItemByFileName(const QString& fileName) const;

	bool eventFilter(QObject*, QEvent*);
};


#endif
