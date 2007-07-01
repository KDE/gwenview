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
#ifndef TRANSFORMIMAGEOPERATION_H
#define TRANSFORMIMAGEOPERATION_H

#include "gwenviewlib_export.h"

// Qt

// KDE

// Local
#include "abstractimageoperation.h"

#include "orientation.h"

namespace Gwenview {


class TransformImageOperationPrivate;
class GWENVIEWLIB_EXPORT TransformImageOperation : public AbstractImageOperation {
public:
	TransformImageOperation(Orientation);
	~TransformImageOperation();

	virtual void apply(Document::Ptr);

private:
	TransformImageOperationPrivate* const d;
};


} // namespace

#endif /* TRANSFORMIMAGEOPERATION_H */
