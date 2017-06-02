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
#include "resizeimageoperation.h"

// Qt
#include <QImage>
#include <QDebug>

// KDE
#include <KLocalizedString>

// Local
#include "document/abstractdocumenteditor.h"
#include "document/document.h"
#include "document/documentjob.h"

namespace Gwenview
{

struct ResizeImageOperationPrivate
{
    QSize mSize;
    QImage mOriginalImage;
};

class ResizeJob : public ThreadedDocumentJob
{
public:
    ResizeJob(const QSize& size)
        : mSize(size)
    {}

    void threadedStart() Q_DECL_OVERRIDE
    {
        if (!checkDocumentEditor()) {
            return;
        }
        QImage image = document()->image();
        image = image.scaled(mSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        document()->editor()->setImage(image);
        setError(NoError);
    }

private:
    QSize mSize;
};

ResizeImageOperation::ResizeImageOperation(const QSize& size)
: d(new ResizeImageOperationPrivate)
{
    d->mSize = size;
    setText(i18nc("(qtundo-format)", "Resize"));
}

ResizeImageOperation::~ResizeImageOperation()
{
    delete d;
}

void ResizeImageOperation::redo()
{
    d->mOriginalImage = document()->image();
    redoAsDocumentJob(new ResizeJob(d->mSize));
}

void ResizeImageOperation::undo()
{
    if (!document()->editor()) {
        qWarning() << "!document->editor()";
        return;
    }
    document()->editor()->setImage(d->mOriginalImage);
}

} // namespace
