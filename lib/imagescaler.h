/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef IMAGESCALER_H
#define IMAGESCALER_H

// Qt
#include <QObject>

#include "gwenviewlib_export.h"

class QImage;
class QRegion;

namespace Gwenview {

class ImageScalerPrivate;
class GWENVIEWLIB_EXPORT ImageScaler : public QObject {
	Q_OBJECT
public:
	ImageScaler(QObject* parent=0);
	~ImageScaler();
	void setImage(const QImage&);
	void setZoom(qreal);
	void setRegion(const QRegion&);
	void addRegion(const QRegion&);
	void start() const;

	bool isRunning();

Q_SIGNALS:
	void scaledRect(int left, int top, const QImage&);

private Q_SLOTS:
	void processChunk();

private:
	ImageScalerPrivate * const d;
};

} // namespace


#endif /* IMAGESCALER_H */
