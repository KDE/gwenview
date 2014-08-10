// vim: set tabstop=4 shiftwidth=4 expandtab:
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
// Self
#include "transformimageoperation.h"

// Qt

// KDE
#include <QDebug>
#include <KLocale>

// Local
#include "document/abstractdocumenteditor.h"

namespace Gwenview
{

struct TransformImageOperationPrivate
{
    Orientation mOrientation;
};

TransformJob::TransformJob(Orientation orientation)
: mOrientation(orientation)
{

}

void TransformJob::threadedStart()
{
    if (!checkDocumentEditor()) {
        return;
    }
    document()->editor()->applyTransformation(mOrientation);
    setError(NoError);
}

TransformImageOperation::TransformImageOperation(Orientation orientation)
: d(new TransformImageOperationPrivate)
{
    d->mOrientation = orientation;
    switch (d->mOrientation) {
    case ROT_90:
        setText(i18nc("(qtundo-format)", "Rotate Right"));
        break;
    case ROT_270:
        setText(i18nc("(qtundo-format)", "Rotate Left"));
        break;
    case HFLIP:
        setText(i18nc("(qtundo-format)", "Mirror"));
        break;
    case VFLIP:
        setText(i18nc("(qtundo-format)", "Flip"));
        break;
    default:
        // We should not get there because the transformations listed above are
        // the only one available from the UI. Define a default text nevertheless.
        setText(i18nc("(qtundo-format)", "Transform"));
        break;
    }
}

TransformImageOperation::~TransformImageOperation()
{
    delete d;
}

void TransformImageOperation::redo()
{
    redoAsDocumentJob(new TransformJob(d->mOrientation));
}

void TransformImageOperation::undo()
{
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
    document()->enqueueJob(new TransformJob(orientation));
}

} // namespace
