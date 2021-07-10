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
#include "abstractimageoperation.h"

// Qt
#include <QTimer>
#include <QUrl>

// KF
#include <KJob>

// Local
#include "document/documentfactory.h"
#include "document/documentjob.h"

namespace Gwenview
{
class ImageOperationCommand : public QUndoCommand
{
public:
    ImageOperationCommand(AbstractImageOperation *op)
        : mOp(op)
    {
    }

    ~ImageOperationCommand() override
    {
        delete mOp;
    }

    void undo() override
    {
        mOp->undo();
    }

    void redo() override
    {
        mOp->redo();
    }

private:
    AbstractImageOperation *const mOp;
};

struct AbstractImageOperationPrivate {
    QString mText;
    QUrl mUrl;
    ImageOperationCommand *mCommand;
};

AbstractImageOperation::AbstractImageOperation()
    : d(new AbstractImageOperationPrivate)
{
}

AbstractImageOperation::~AbstractImageOperation()
{
    delete d;
}

void AbstractImageOperation::applyToDocument(Document::Ptr doc)
{
    d->mUrl = doc->url();

    d->mCommand = new ImageOperationCommand(this);
    d->mCommand->setText(d->mText);
    // QUndoStack::push() executes command by calling its redo() function
    doc->undoStack()->push(d->mCommand);
}

Document::Ptr AbstractImageOperation::document() const
{
    Document::Ptr doc = DocumentFactory::instance()->load(d->mUrl);
    doc->startLoadingFullImage();
    return doc;
}

void AbstractImageOperation::finish(bool ok)
{
    if (ok) {
        // Give QUndoStack time to update in case the redo/undo is executed immediately
        // (e.g. undo crop just sets the previous image)
        QTimer::singleShot(0, document().data(), &Document::imageOperationCompleted);
    } else {
        // Remove command from undo stack without executing undo()
        d->mCommand->setObsolete(true);
        document()->undoStack()->undo();
    }
}

void AbstractImageOperation::finishFromKJob(KJob *job)
{
    finish(job->error() == KJob::NoError);
}

void AbstractImageOperation::setText(const QString &text)
{
    d->mText = text;
}

void AbstractImageOperation::redoAsDocumentJob(DocumentJob *job)
{
    connect(job, &KJob::result, this, &AbstractImageOperation::finishFromKJob);
    document()->enqueueJob(job);
}

} // namespace
