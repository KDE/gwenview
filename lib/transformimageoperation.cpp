// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "transformimageoperation.h"

// Qt

// KDE
#include <klocale.h>

// Local

namespace Gwenview {


struct TransformImageOperationPrivate {
	Orientation mOrientation;
};


TransformImageOperation::TransformImageOperation(Orientation orientation)
: d(new TransformImageOperationPrivate) {
	d->mOrientation = orientation;
	switch (d->mOrientation) {
	case ROT_90:
		setText(i18n("Rotate Right"));
		break;
	case ROT_270:
		setText(i18n("Rotate Left"));
		break;
	case HFLIP:
		setText(i18n("Mirror"));
		break;
	case VFLIP:
		setText(i18n("Flip"));
		break;
	default:
		// We should not get there because the transformations listed above are
		// the only one available from the UI. Define a default text nevertheless.
		setText(i18n("Transform"));
		break;
	}
}


TransformImageOperation::~TransformImageOperation() {
	delete d;
}


void TransformImageOperation::redo() {
	document()->applyTransformation(d->mOrientation);
}


void TransformImageOperation::undo() {
	Orientation orientation;
	switch (d->mOrientation) {
	case ROT_90:
		orientation = ROT_270;
		break;
	case ROT_270:
		orientation = ROT_90;
		break;
	default:
		orientation = d->mOrientation;
		break;
	}
	document()->applyTransformation(orientation);
}


} // namespace
