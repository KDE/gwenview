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
#ifndef __gvdirpart_h__
#define __gvdirpart_h__

#include <kparts/part.h>
#include <kparts/browserextension.h>

// Forward declarations
class QPoint;
class QSplitter;
class KAboutData;
class KAction;
class KToggleAction;

namespace Gwenview {

class ImageView;
class FileViewController;
class Document;
class SlideShow;

class GVDirPart;

/**
 * The browser extension is an attribute of GVImagePart and provides
 * some services to Konqueror.  All Konqueror KParts have one.
 */
class GVDirPartBrowserExtension: public KParts::BrowserExtension {
	Q_OBJECT

public:
	GVDirPartBrowserExtension(GVDirPart* viewPart, const char* name=0L);
	~GVDirPartBrowserExtension();

public slots:
	void updateActions();

	void trash();
	void del();

	void print();

private slots:
	void openFileViewContextMenu(const QPoint&, bool onItem);
	void openImageViewContextMenu(const QPoint&);
	
private:
	GVDirPart* mGVDirPart;
};

/**
 * A Read Only KPart to browse directories and their images using Gwenview
 */
class GVDirPart : public KParts::ReadOnlyPart {
	Q_OBJECT
public:
	GVDirPart(QWidget*, const char*, QObject*, const char*, const QStringList &);
	virtual ~GVDirPart();

	/**
	 * Return information about the part
	 */
	static KAboutData* createAboutData();

	/**
	 * Returns the name of the current file in the pixmap
	 */
	KURL pixmapURL();

	/**
	 * Print the image being viewed if there is one
	 */
	void print();

	FileViewController* fileViewController() const { return mFileViewController; }


protected:
	void partActivateEvent(KParts::PartActivateEvent* event);

	/**
	 * Unused because openURL() is implemented but required to be
	 * implemented.
	 */
	virtual bool openFile();
	
	/**
	 * Tell the widgets the URL to browse.  Sets the window
	 * caption and saves URL to m_url (important for history and
	 * others).
	 */
	virtual bool openURL(const KURL& url);

protected slots:
	/**
	 * Turns the slide show on or off
	 */
	void toggleSlideShow();

	/**
	 * Sets Konqueror's caption, statusbar and emits completed().
	 * Called by loaded() signal in GVDocument
	 */
	void loaded(const KURL& url);

	/**
	 * Rotates the current image 90 degrees counter clockwise
	 */
	void rotateLeft();

	/**
	 * Rotates the current image 90 degrees clockwise
	 */
	void rotateRight();

	void directoryChanged(const KURL& dirURL);

	void slotSlideShowChanged( const KURL& );

protected:
	/**
	 * The component's widget, contains the files view on the left
	 * and scroll view on the right.
	 */
	QSplitter* mSplitter;

	/**
	 * Scroll widget
	 */
	ImageView* mImageView;

	/**
	 * Holds the image
	 */
	Document* mDocument;

	/**
	 * Shows the directory's files and folders
	 */

	FileViewController* mFileViewController;

	/**
	 * This inherits from KParts::BrowserExtention and supplies
	 * some extra functionality to Konqueror.
	 */
	GVDirPartBrowserExtension* mBrowserExtension;

	/**
	 * Action turns on slide show
	 */
	KToggleAction* mToggleSlideShow;
	SlideShow* mSlideShow;
};

} // namespace
#endif
