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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __gvimagepart_h__
#define __gvimagepart_h__

#include <kparts/part.h>

// Forward declarations
class KAboutData;
class KAction;
class GVScrollPixmapView;
class GVPixmap;
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

//protected slots:
 public slots:
	void contextMenu();
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
	void GVImagePart::print();

 protected:
	/** Open the file whose path is stored in the member variable
	 * m_file and return true on success, false on failure.
	 */
	virtual bool openFile();

 protected slots:
	/**
	 * Sets Konqueror's caption with setWindowCaption()
	 * called by loaded() signal in GVPixmap
	 */
	void setKonquerorWindowCaption(const KURL& url, const QString& filename);

 protected:
        /**
	 * The component's widget
	 */
	GVScrollPixmapView* mPixmapView;

	/**
	 * Holds the image
	 */
	GVPixmap* mGVPixmap;

	/**
	 * This inherits from KParts::BrowserExtention and supplies
	 * some extra functionality to Konqueror.
	 */
	GVImagePartBrowserExtension* mBrowserExtension;
};

#endif
