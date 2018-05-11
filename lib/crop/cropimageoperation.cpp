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
#include "cropimageoperation.h"

// Qt
#include <QImage>

// KDE
#include <QDebug>
#include <KLocalizedString>

// Local
#include "document/document.h"
#include "document/documentjob.h"
#include "document/abstractdocumenteditor.h"

namespace Gwenview
{

class CropJob : public ThreadedDocumentJob
{
public:
    CropJob(const QRect& rect)
        : mRect(rect)
    {}

    void threadedStart() Q_DECL_OVERRIDE
    {
        if (!checkDocumentEditor()) {
            return;
        }
        const QImage src = document()->image();
        const QImage dst = src.copy(mRect);
        document()->editor()->setImage(dst);
        setError(NoError);
    }

private:
    QRect mRect;
};

struct CropImageOperationPrivate
{
    QRect mRect;
    QImage mOriginalImage;
};

CropImageOperation::CropImageOperation(const QRect& rect)
: d(new CropImageOperationPrivate)
{
    d->mRect = rect;
    setText(i18n("Crop"));
}

CropImageOperation::~CropImageOperation()
{
    delete d;
}

void CropImageOperation::redo()
{
    d->mOriginalImage = document()->image();
    redoAsDocumentJob(new CropJob(d->mRect));
}

void CropImageOperation::undo()
{
    if (!document()->editor()) {
        qWarning() << "!document->editor()";
        return;
    }
    document()->editor()->setImage(d->mOriginalImage);
    finish(true);
}

} // namespace
