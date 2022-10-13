/*
Gwenview: an image viewer
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
#include "annotateoperation.h"

// Qt
#include <QImage>

// KF
#include <KLocalizedString>

// Local
#include "document/abstractdocumenteditor.h"
#include "document/document.h"
#include "gwenview_lib_debug.h"

namespace Gwenview
{
class AnnotateOperationPrivate
{
public:
    AnnotateOperationPrivate()
    {
    }

    QImage mNewImage;
    QImage mOriginalImage;
};

AnnotateOperation::AnnotateOperation(const QImage &image)
    : d(new AnnotateOperationPrivate())
{
    d->mNewImage = image;
    setText(i18nc("@action:intoolbar", "Annotate"));
}

AnnotateOperation::~AnnotateOperation()
{
    delete d;
}

void AnnotateOperation::redo()
{
    if (!document()->editor()) {
        qCWarning(GWENVIEW_LIB_LOG) << "!document->editor()";
        return;
    }

    d->mOriginalImage = document()->image();
    document()->editor()->setImage(d->mNewImage);
    finish(true);
}

void AnnotateOperation::undo()
{
    if (!document()->editor()) {
        qCWarning(GWENVIEW_LIB_LOG) << "!document->editor()";
        return;
    }
    document()->editor()->setImage(d->mOriginalImage);
    finish(true);
}

} // namespace
