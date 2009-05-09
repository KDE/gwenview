// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef ABSTRACTIMAGEVIEWTOOL_H
#define ABSTRACTIMAGEVIEWTOOL_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>

// KDE

// Local

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QPainter;

namespace Gwenview {

class ImageView;

class AbstractImageViewToolPrivate;
class GWENVIEWLIB_EXPORT AbstractImageViewTool : public QObject {
public:
	AbstractImageViewTool(ImageView* view);
	virtual ~AbstractImageViewTool();

	ImageView* imageView() const;

	virtual void paint(QPainter*) {}

	virtual void mousePressEvent(QMouseEvent*) {}
	virtual void mouseMoveEvent(QMouseEvent*) {}
	virtual void mouseReleaseEvent(QMouseEvent*) {}
	virtual void wheelEvent(QWheelEvent*) {}
	virtual void keyPressEvent(QKeyEvent*) {}
	virtual void keyReleaseEvent(QKeyEvent*) {}

	virtual void toolActivated() {}
	virtual void toolDeactivated() {}

private:
	AbstractImageViewToolPrivate * const d;
};


} // namespace

#endif /* ABSTRACTIMAGEVIEWTOOL_H */
