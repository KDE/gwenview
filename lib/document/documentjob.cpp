// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include "documentjob.h"

// Qt
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QApplication>
#include "gwenview_lib_debug.h"

// KDE
#include <KDialogJobUiDelegate>
#include <KLocalizedString>

// Local

namespace Gwenview
{

struct DocumentJobPrivate
{
    Document::Ptr mDoc;
};

DocumentJob::DocumentJob()
: KCompositeJob(nullptr)
, d(new DocumentJobPrivate)
{
    KDialogJobUiDelegate* delegate = new KDialogJobUiDelegate;
    delegate->setWindow(qApp->activeWindow());
    delegate->setAutoErrorHandlingEnabled(true);
    setUiDelegate(delegate);
}

DocumentJob::~DocumentJob()
{
    delete d;
}

Document::Ptr DocumentJob::document() const
{
    return d->mDoc;
}

void DocumentJob::setDocument(const Document::Ptr& doc)
{
    d->mDoc = doc;
}

void DocumentJob::start()
{
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
}

bool DocumentJob::checkDocumentEditor()
{
    if (!document()->editor()) {
        setError(NoDocumentEditorError);
        setErrorText(i18nc("@info", "Gwenview cannot edit this kind of image."));
        return false;
    }
    return true;
}

void ThreadedDocumentJob::doStart()
{
    QFuture<void> future = QtConcurrent::run(this, &ThreadedDocumentJob::threadedStart);
    QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
    connect(watcher, SIGNAL(finished()), SLOT(emitResult()));
    watcher->setFuture(future);
}

} // namespace
