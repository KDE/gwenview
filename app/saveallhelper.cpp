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
#include <QSet>
#include <QStringList>

// KDE
#include <KLocale>
#include <KMessageBox>
#include <KProgressDialog>
#include <QUrl>

// Local
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/document/documentjob.h>

namespace Gwenview
{

struct SaveAllHelperPrivate
{
    QWidget* mParent;
    KProgressDialog* mProgressDialog;
    QSet<DocumentJob*> mJobSet;
    QStringList mErrorList;
};

SaveAllHelper::SaveAllHelper(QWidget* parent)
: d(new SaveAllHelperPrivate)
{
    d->mParent = parent;
    d->mProgressDialog = new KProgressDialog(parent);
    connect(d->mProgressDialog, &KProgressDialog::cancelClicked, this, &SaveAllHelper::slotCanceled);
    d->mProgressDialog->setLabelText(i18nc("@info:progress saving all image changes", "Saving..."));
    d->mProgressDialog->setButtonText(i18n("&Stop"));
    d->mProgressDialog->progressBar()->setMinimum(0);
}

SaveAllHelper::~SaveAllHelper()
{
    delete d;
}

void SaveAllHelper::save()
{
    QList<QUrl> list = DocumentFactory::instance()->modifiedDocumentList();
    d->mProgressDialog->progressBar()->setRange(0, list.size());
    d->mProgressDialog->progressBar()->setValue(0);
    Q_FOREACH(const QUrl &url, list) {
        Document::Ptr doc = DocumentFactory::instance()->load(url);
        DocumentJob* job = doc->save(url, doc->format());
        connect(job, &DocumentJob::result, this, &SaveAllHelper::slotResult);
        d->mJobSet << job;
    }

    d->mProgressDialog->exec();

    // Done, show message if necessary
    if (d->mErrorList.count() > 0) {
        QString msg = i18ncp("@info", "One document could not be saved:", "%1 documents could not be saved:", d->mErrorList.count());
        msg += "<ul>";
        Q_FOREACH(const QString & item, d->mErrorList) {
            msg += "<li>" + item + "</li>";
        }
        msg += "</ul>";
        KMessageBox::sorry(d->mParent, msg);
    }
}

void SaveAllHelper::slotCanceled()
{
    Q_FOREACH(DocumentJob * job, d->mJobSet) {
        job->kill();
    }
}

void SaveAllHelper::slotResult(KJob* _job)
{
    DocumentJob* job = static_cast<DocumentJob*>(_job);
    if (job->error()) {
        QUrl url = job->document()->url();
        QString name = url.fileName().isEmpty() ? url.toDisplayString() : url.fileName();
        d->mErrorList << i18nc("@info %1 is the name of the document which failed to save, %2 is the reason for the failure",
                               "<filename>%1</filename>: %2", name, job->errorString());
    }
    d->mJobSet.remove(job);
    QProgressBar* bar = d->mProgressDialog->progressBar();
    bar->setValue(bar->value() + 1);
}

} // namespace
