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
#include "saveallhelper.h"

// Qt
#include <QFuture>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QSet>
#include <QStringList>
#include <QUrl>

// KF
#include <KLocalizedString>
#include <KMessageBox>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/document/documentjob.h>

namespace Gwenview
{
struct SaveAllHelperPrivate {
    QWidget *mParent = nullptr;
    QProgressDialog *mProgressDialog = nullptr;
    QSet<DocumentJob *> mJobSet;
    QStringList mErrorList;
};

SaveAllHelper::SaveAllHelper(QWidget *parent)
    : d(new SaveAllHelperPrivate)
{
    d->mParent = parent;
    d->mProgressDialog = new QProgressDialog(parent);
    connect(d->mProgressDialog, &QProgressDialog::canceled, this, &SaveAllHelper::slotCanceled);
    d->mProgressDialog->setLabelText(i18nc("@info:progress saving all image changes", "Saving..."));
    d->mProgressDialog->setCancelButtonText(i18n("&Stop"));
    d->mProgressDialog->setMinimum(0);
}

SaveAllHelper::~SaveAllHelper()
{
    delete d;
}

void SaveAllHelper::save()
{
    const QList<QUrl> list = DocumentFactory::instance()->modifiedDocumentList();
    d->mProgressDialog->setRange(0, list.size());
    d->mProgressDialog->setValue(0);
    for (const QUrl &url : list) {
        Document::Ptr doc = DocumentFactory::instance()->load(url);
        DocumentJob *job = doc->save(url, doc->format());
        connect(job, &DocumentJob::result, this, &SaveAllHelper::slotResult);
        d->mJobSet << job;
    }

    d->mProgressDialog->exec();

    // Done, show message if necessary
    if (!d->mErrorList.isEmpty()) {
        QString msg = i18ncp("@info", "One document could not be saved:", "%1 documents could not be saved:", d->mErrorList.count());
        msg += QLatin1String("<ul>");
        for (const QString &item : qAsConst(d->mErrorList)) {
            msg += "<li>" + item + "</li>";
        }
        msg += QLatin1String("</ul>");
        KMessageBox::error(d->mParent, msg);
    }
}

void SaveAllHelper::slotCanceled()
{
    for (DocumentJob *job : qAsConst(d->mJobSet)) {
        job->kill();
    }
}

void SaveAllHelper::slotResult(KJob *_job)
{
    auto job = static_cast<DocumentJob *>(_job);
    if (job->error()) {
        const QUrl url = job->document()->url();
        const QString name = url.fileName().isEmpty() ? url.toDisplayString() : url.fileName();
        d->mErrorList << xi18nc("@info %1 is the name of the document which failed to save, %2 is the reason for the failure",
                                "<filename>%1</filename>: %2",
                                name,
                                kxi18n(qPrintable(job->errorString())));
    }
    d->mJobSet.remove(job);
    d->mProgressDialog->setValue(d->mProgressDialog->value() + 1);
}

} // namespace
