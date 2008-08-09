// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef SVGVIEWADAPTER_H
#define SVGVIEWADAPTER_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/abstractdocumentviewadapter.h>

namespace Gwenview {


class SvgViewAdapterPrivate;
class GWENVIEWLIB_EXPORT SvgViewAdapter : public AbstractDocumentViewAdapter {
	Q_OBJECT
public:
	SvgViewAdapter(QWidget*);
	~SvgViewAdapter();

	virtual void installEventFilterOnViewWidgets(QObject*);

	virtual void setDocument(Document::Ptr);

	virtual Document::Ptr document() const;

private:
	SvgViewAdapterPrivate* const d;
};


} // namespace

#endif /* SVGVIEWADAPTER_H */
