// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef ABSTRACTIMAGEVIEWTOOL_H
#define ABSTRACTIMAGEVIEWTOOL_H

// Qt
#include <QObject>

// KDE

// Local

class QPainter;

namespace Gwenview {

class ImageView;

class AbstractImageViewToolPrivate;
class AbstractImageViewTool : public QObject {
public:
	AbstractImageViewTool(QObject* parent);
	virtual ~AbstractImageViewTool();

	void setImageView(ImageView* imageView);
	ImageView* imageView() const;

	virtual void paint(QPainter*) = 0;

private:
	AbstractImageViewToolPrivate * const d;
};


} // namespace

#endif /* ABSTRACTIMAGEVIEWTOOL_H */
