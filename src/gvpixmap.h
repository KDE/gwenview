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
#ifndef GVPIXMAP_H
#define GVPIXMAP_H

// Qt includes
#include <qobject.h>
#include <qimage.h>

// KDE includes
#include <kurl.h>


/**
 * A pixmap class with zooming capacities
 * Emits a signal when it changes
 */
class GVPixmap : public QObject {
Q_OBJECT
public:
	GVPixmap(QObject*);
	~GVPixmap();
	void reset();

	// Properties
	const QImage& image() const { return mImage; }
	KURL url() const;
	const KURL& dirURL() const { return mDirURL; }
	const QString& filename() const { return mFilename; }
	int width() const { return mImage.width(); }
	int height() const { return mImage.height(); }
	bool isNull() const { return mImage.isNull(); }
	const QString& imageFormat() const { return mImageFormat; }


public slots:
	void setURL(const KURL&);
	void setDirURL(const KURL&);
	void setFilename(const QString&);

	void save();
	void saveAs();

	// "Image manipulation"
	void rotateLeft();
	void rotateRight();
	void mirror();
	void flip();
	
signals:
	/**
	 * Emitted when the class starts to load the image.
	 */
	void loading();

	/**
	 * Emitted when the class has finished loading the image.
	 * Also emitted if the image could not be loaded.
	 */
	void urlChanged(const KURL&,const QString&);

	/**
	 * Emitted when the image is modified.
	 */
	void modified();

private:
	QImage mImage;
	KURL mDirURL;
	QString mFilename;
	QString mImageFormat;

	bool load();
	bool saveInternal(const KURL&,const QString& format);
};


#endif
