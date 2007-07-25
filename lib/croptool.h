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
#ifndef CROPTOOL_H
#define CROPTOOL_H

// Qt

// KDE

// Local
#include "abstractimageviewtool.h"

class QRect;

namespace Gwenview {

class CropToolPrivate;
class CropTool : public AbstractImageViewTool {
	Q_OBJECT
public:
	CropTool(QObject* parent);
	~CropTool();

	void setRect(const QRect&);

	virtual void paint(QPainter*);

Q_SIGNALS:
	void rectUpdated(const QRect&);

private:
	CropToolPrivate* const d;
};


} // namespace

#endif /* CROPTOOL_H */
