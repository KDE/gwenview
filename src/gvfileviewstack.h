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
#ifndef GVFILEVIEWCONTAINER_H
#define GVFILEVIEWCONTAINER_H

// Qt includes
#include <qobject.h>
#include <qwidgetstack.h>

// KDE includes
#include <kurl.h>

class QIconViewItem;
class QListViewItem;
class QPopupMenu;

class KAccel;
class KAction;
class KActionCollection;
class KConfig;
class KDirLister;
class KRadioAction;

class GVFileViewBase;
class GVFileDetailView;
class FileThumbnailView;


class GVFileViewStack : public QWidgetStack {
Q_OBJECT

public:
	enum Mode { FileList, Thumbnail};

	GVFileViewStack(QWidget* parent,KActionCollection*);
	~GVFileViewStack();

	void plugActionsToAccel(KAccel*);

	// Config
	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;

	void setAutoLoadImage(bool);
	bool autoLoadImage() const { return mAutoLoadImage; }

	// Properties
	void setMode(Mode);
	
	QString filename() const;
	KURL url() const;
	uint fileCount() const;
	bool showDirs() const;
	void setShowDirs(bool);
	uint selectionSize() const;
	
	FileThumbnailView* fileThumbnailView() const { return mFileThumbnailView; }
	
	KAction* selectFirst() const { return mSelectFirst; }
	KAction* selectLast() const { return mSelectLast; }
	KAction* selectPrevious() const { return mSelectPrevious; }
	KAction* selectNext() const { return mSelectNext; }
	KRadioAction* noThumbnails() const { return mNoThumbnails; }
	KRadioAction* smallThumbnails() const { return mSmallThumbnails; }
	KRadioAction* medThumbnails() const { return mMedThumbnails; }
	KRadioAction* largeThumbnails() const { return mLargeThumbnails; }

public slots:
	void setURL(const KURL&,const QString&);

	void selectFilename(QString filename);

	void slotSelectFirst();
	void slotSelectLast();
	void slotSelectPrevious();
	void slotSelectNext();

	// Stop thumbnail generation
	void cancel();

	void openWithEditor();
	void openParentDir();
	void showFileProperties();
	void deleteFiles();
	void renameFile();
	void copyFiles();
	void moveFiles();


signals:
	void urlChanged(const KURL&);
	void completed();
	void canceled();

	// Thumbnail view signals
	void updateStarted(int);
	void updateEnded();
	void updatedOneThumbnail();


private slots:
	
	// Used to enter directories
	void viewExecuted();

	// Used to change the current image
	void viewClicked();

	// Context menu
	void openContextMenu(const QPoint& pos);
	void openContextMenu(KListView*, QListViewItem*, const QPoint&);
	void openContextMenu(QIconViewItem*,const QPoint&);

	// Get called by the thumbnail size radio actions
	void updateThumbnailSize();

	// Dir lister slots
	void dirListerDeleteItem(KFileItem* item);
	void dirListerNewItems(const KFileItemList& items);
	void dirListerRefreshItems(const KFileItemList&);
	void dirListerClear();
	void dirListerStarted();
	void dirListerCanceled();
	void dirListerCompleted();

private:
	Mode mMode;
	GVFileDetailView* mFileDetailView;
	FileThumbnailView* mFileThumbnailView;
	KDirLister* mDirLister;
	KURL mDirURL;

	// Our actions
	KAction* mSelectFirst;
	KAction* mSelectLast;
	KAction* mSelectPrevious;
	KAction* mSelectNext;
	
	KRadioAction* mNoThumbnails;
	KRadioAction* mSmallThumbnails;
	KRadioAction* mMedThumbnails;
	KRadioAction* mLargeThumbnails;

	// configurable settings
	bool mAutoLoadImage;
	bool mShowDirs;

	// Temp data used by the dir lister
	bool mThumbnailsNeedUpdate;
	QString mFilenameToSelect;

	GVFileViewBase* currentFileView() const;
	void emitURLChanged();
	void updateActions();
	void initDirListerFilter();
	KURL::List selectedURLs() const;
	
	KFileItem* findFirstImage() const;
	KFileItem* findLastImage() const;
	KFileItem* findPreviousImage() const;
	KFileItem* findNextImage() const;
};


#endif
