// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "importer.moc"

// Qt
#include <QFileInfo>

// KDE
#include <kdatetime.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <ktempdir.h>
#include <kurl.h>
#include <kio/copyjob.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>

// stdc++
#include <memory>

// Local
#include <lib/timeutils.h>

namespace Gwenview {


struct ImporterPrivate {
	Importer* q;
	KUrl::List mUrlList;
	QWidget* mAuthWindow;
	std::auto_ptr<KTempDir> mDestImportDir;
	bool mAtLeastOneRenameFailure;
	int mProgress;

	bool createImportDir(const KUrl url) {
		if (!url.isLocalFile()) {
			kWarning() << "FIXME: Support remote urls";
			return false;
		}
		if (!KStandardDirs::makeDir(url.toLocalFile())) {
			kWarning() << "FIXME: Handle destination dir creation failure";
			return false;
		}
		mDestImportDir.reset(new KTempDir(url.toLocalFile() + "/.gwenview_importer-"));
		mDestImportDir->setAutoRemove(false);
		return mDestImportDir->status() == 0;
	}

	void importNext() {
		kDebug() << mUrlList;
		if (mUrlList.empty()) {
			q->finalizeImport();
			return;
		}
		KUrl url = mUrlList.takeFirst();
		KIO::Job* job = KIO::copy(url, KUrl(mDestImportDir->name()), KIO::HideProgressInfo);
		if (job->ui()) {
			job->ui()->setWindow(mAuthWindow);
		}
		QObject::connect(job, SIGNAL(result(KJob*)),
			q, SLOT(slotResult(KJob*)));
	}

	void renameImportedUrl(const KUrl& url) {
		KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url, true /* delayedMimeTypes */);
		KDateTime dateTime = TimeUtils::dateTimeForFileItem(item);
		KUrl newUrl = url;
		newUrl.cd("..");
		QFileInfo info(url.fileName());
		newUrl.setFileName(dateTime.toString("%yyyy-%MM-%dd_%hh-%mm-%ss")
			+ '.' + info.completeSuffix());

		KIO::Job* job = KIO::rename(url, newUrl, KIO::HideProgressInfo);
		if (!KIO::NetAccess::synchronousRun(job, mAuthWindow)) {
			kWarning() << "Renaming of" << url << "to" << newUrl << "failed";
			mAtLeastOneRenameFailure = true;
		}
		q->advance();
	}
};


Importer::Importer(QWidget* parent)
: QObject(parent)
, d(new ImporterPrivate) {
	d->q = this;
	d->mAuthWindow = parent;
}

Importer::~Importer() {
	delete d;
}

void Importer::start(const KUrl::List& list, const KUrl& destination) {
	kDebug();
	d->mProgress = 0;
	d->mUrlList = list;
	d->mAtLeastOneRenameFailure = false;

	progressChanged(0);
	maximumChanged(d->mUrlList.count());

	if (!d->createImportDir(destination)) {
		kWarning() << "FIXME: Handle failure in import dir creation";
		return;
	}
	d->importNext();
}

void Importer::slotResult(KJob* _job) {
	kDebug();
	KIO::CopyJob* job = static_cast<KIO::CopyJob*>(_job);
	KUrl url = job->srcUrls().first();
	if (!job->error()) {
		// FIXME: What do we do with failed urls?
		advance();
		d->importNext();
		return;
	}

	d->renameImportedUrl(url);
}

void Importer::finalizeImport() {
	kDebug();
	if (d->mAtLeastOneRenameFailure) {
		kWarning() << "FIXME: Handle rename failures";
	} else {
		d->mDestImportDir->unlink();
	}
	importFinished();
}

void Importer::advance() {
	++d->mProgress;
	progressChanged(d->mProgress);
}

} // namespace
