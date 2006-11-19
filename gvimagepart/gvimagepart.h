/*
Copyright 2004 Jonathan Riddell <jr@jriddell.org>

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
Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.

*/
#ifndef __gvimagepart_h__
#define __gvimagepart_h__

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <ktempfile.h>

// Forward declarations
class QFile;
class QPoint;

class KAboutData;
class KAction;
class KDirLister;
class KFileItem;

namespace Gwenview {
class ImageView;
class Document;
class ImageLoader;

class GVImagePart;

/**
 * The browser extension is an attribute of GVImagePart and provides
 * some services to Konqueror.  All Konqueror KParts have one.
 */
class GVImagePartBrowserExtension: public KParts::BrowserExtension {
	Q_OBJECT

public:
	GVImagePartBrowserExtension(GVImagePart* viewPart, const char* name=0L);
	~GVImagePartBrowserExtension();

public slots:
	void print();

private:
	GVImagePart* mGVImagePart;

};

/**
 * A Read Only KPart to view images using Gwenview
 */
class GVImagePart : public KParts::ReadOnlyPart {
	Q_OBJECT
public:
	GVImagePart(QWidget*, const char*, QObject*, const char*, const QStringList &);
	virtual ~GVImagePart();

	/**
	 * Return information about the part
	 */
	static KAboutData* createAboutData();

	/**
	 * Returns m_file
	 */
	QString filePath();

	/**
	 * Print the image being viewed
	 */
	void print();

public slots:
	virtual bool openURL(const KURL& url);

protected slots:
	virtual bool openFile() { return false; }

	/**
	 * Rotates the current image 90 degrees counter clockwise
	 */
	void rotateLeft();

	/**
	 * Rotates the current image 90 degrees clockwise
	 */
	void rotateRight();

protected:
	virtual void partActivateEvent(KParts::PartActivateEvent* event);
	virtual void guiActivateEvent( KParts::GUIActivateEvent* event);

private slots:

	void dirListerClear();

	void dirListerNewItems( const KFileItemList& );

	void dirListerDeleteItem(KFileItem*);

	void slotSelectNext();
	void slotSelectPrevious();

	void prefetchDone();

	void slotLoading();
	void slotLoaded(const KURL& url);
	
    void openContextMenu(const QPoint&);

	void saveAs();
	
	void showJobError(KIO::Job* job);

private:

	void updateNextPrevious();
	KURL nextURL() const;
	KURL previousURL() const;
	void saveOriginalAs();

	/**
	 * The component's widget
	 */
	ImageView* mImageView;

	/**
	 * Holds the image
	 */
	Document* mDocument;

	/**
	 * This inherits from KParts::BrowserExtention and supplies
	 * some extra functionality to Konqueror.
	 */
	GVImagePartBrowserExtension* mBrowserExtension;

	// for the next/previous actions
	KDirLister* mDirLister;

	KAction* mNextImage;
	KAction* mPreviousImage;
	// alphabetically sorted filenames of images in the picture's directory
	QStringList mImagesInDirectory;

	ImageLoader* mPrefetch;
	enum LastDirection { DirectionUnknown, DirectionNext, DirectionPrevious };
	LastDirection mLastDirection; // used for prefetching
};


/**
 * This simple helper class uploads data to a remote URL asynchronously
 */
class DataUploader : public QObject {
	Q_OBJECT
public:
	DataUploader(QWidget* dialogParent, const QByteArray& data, const KURL& destURL);

private slots:
	void slotJobFinished(KIO::Job*);

private:
	KTempFile mTempFile;
	QWidget* mDialogParent;
};

} // namespace
#endif
