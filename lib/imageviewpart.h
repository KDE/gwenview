/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef IMAGEVIEWPART_H
#define IMAGEVIEWPART_H

// KDE
#include <kparts/part.h>

// Local
#include "../lib/document/document.h"

#include "gwenviewlib_export.h"

namespace Gwenview {

class Document;
class ImageView;

class GWENVIEWLIB_EXPORT ImageViewPart : public KParts::ReadOnlyPart {
	Q_OBJECT
public:
	ImageViewPart(QObject* parent);
	virtual Document::Ptr document() = 0;
	virtual ImageView* imageView() const = 0;

public Q_SLOTS:
	virtual void loadConfig() = 0;

Q_SIGNALS:
	void resizeRequested(const QSize&);
	void previousImageRequested();
	void nextImageRequested();
};

} // namespace

#endif /* IMAGEVIEWPART_H */
