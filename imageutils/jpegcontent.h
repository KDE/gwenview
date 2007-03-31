// vim: set tabstop=4 shiftwidth=4 noexpandtab
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
#ifndef JPEGCONTENT_H
#define JPEGCONTENT_H

// Qt
#include <qcstring.h>

// Local
#include <imageutils/orientation.h>

#include "../gvcore/libgwenview_export.h"

class QImage;
class QString;
class QFile;

namespace ImageUtils {


class LIBGWENVIEW_EXPORT JPEGContent {
public:
	JPEGContent();
	~JPEGContent();
	
	Orientation orientation() const;
	void resetOrientation();
  
	int dotsPerMeterX() const;
	int dotsPerMeterY() const;

	QSize size() const;

	QString comment() const;
	void setComment(const QString&);
	
	void transform(Orientation);

	QImage thumbnail() const;
	void setThumbnail(const QImage&);

	bool load(const QString& file);
	bool loadFromData(const QByteArray& rawData);
	bool save(const QString& file);
	bool save(QFile*);

private:
	struct Private;
	Private *d;

	JPEGContent(const JPEGContent&);
	void operator=(const JPEGContent&);
    void applyPendingTransformation();
	int dotsPerMeter(const QString& keyName) const;
};


} // namespace


#endif /* JPEGCONTENT_H */
