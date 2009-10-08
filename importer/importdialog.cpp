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
#include "importdialog.moc"

// Qt
#include <QApplication>
#include <QDate>
#include <QStackedWidget>

// KDE
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kio/deletejob.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kservice.h>
#include <kstandardguiitem.h>

// Local
#include "dialogpage.h"
#include "importer.h"
#include "importerconfig.h"
#include "progresspage.h"
#include "thumbnailpage.h"

namespace Gwenview {


class ImportDialogPrivate {
public:
	ImportDialog* q;
	QStackedWidget* mCentralWidget;
	ThumbnailPage* mThumbnailPage;
	ProgressPage* mProgressPage;
	DialogPage* mDialogPage;
	Importer* mImporter;

	void deleteImportedUrls() {
		KUrl::List urls = mImporter->importedUrlList();
		if (urls.count() == 0) {
			return;
		}
		int answer = KMessageBox::questionYesNo(mCentralWidget,
			i18np(
				"One document has been successfully imported.\nDelete it from the device?",
				"%1 documents has been successfully imported.\nDelete them from the device?",
				urls.count()),
			QString(),
			KStandardGuiItem::del(),
			KGuiItem(i18n("Keep"))
			);
		if (answer != KMessageBox::Yes) {
			return;
		}
		while (true) {
			KIO::Job* job = KIO::del(urls);
			if (KIO::NetAccess::synchronousRun(job, q)) {
				break;
			}
			// Deleting failed
			int answer = KMessageBox::warningYesNo(mCentralWidget,
				i18np("Failed to delete the document:\n%2",
					"Failed to delete documents:\n%2",
					urls.count(), job->errorString()),
				QString(),
				KGuiItem(i18n("Retry")),
				KGuiItem(i18n("Ignore"))
				);
			if (answer != KMessageBox::Yes) {
				// Ignore
				break;
			}
		}
	}

	void startGwenview() {
		KService::Ptr service = KService::serviceByDesktopName("gwenview");
		if (!service) {
			kError() << "Could not find gwenview";
		} else {
			KRun::run(*service, KUrl::List() << mThumbnailPage->destinationUrl(), 0 /* window */);
		}
	}

	void showWhatNext() {
		mCentralWidget->setCurrentWidget(mDialogPage);
		mDialogPage->setText(i18n("What do you want to do now?"));
		mDialogPage->removeButtons();
		int gwenview = mDialogPage->addButton(KGuiItem(i18n("View Imported Documents with Gwenview"), "gwenview"));
		int importMore = mDialogPage->addButton(KGuiItem(i18n("Import more Documents")));
		mDialogPage->addButton(KGuiItem(i18n("Quit"), "dialog-cancel"));

		int answer = mDialogPage->exec();
		if (answer == gwenview) {
			startGwenview();
			qApp->quit();
		} else if (answer == importMore) {
			mCentralWidget->setCurrentWidget(mThumbnailPage);
		} else { /* quit */
			qApp->quit();
		}
	}
};


ImportDialog::ImportDialog()
: d(new ImportDialogPrivate) {
	d->q = this;
	d->mImporter = new Importer(this);
	connect(d->mImporter, SIGNAL(error(const QString&)),
		SLOT(showImportError(const QString&)));
	d->mThumbnailPage = new ThumbnailPage;

	KUrl url = ImporterConfig::destinationUrl();
	if (!url.isValid()) {
		url = KUrl::fromPath(KGlobalSettings::picturesPath());
		int year = QDate::currentDate().year();
		url.addPath(QString::number(year));
	}
	d->mThumbnailPage->setDestinationUrl(url);

	d->mProgressPage = new ProgressPage(d->mImporter);

	d->mDialogPage = new DialogPage;

	d->mCentralWidget = new QStackedWidget;
	setCentralWidget(d->mCentralWidget);
	d->mCentralWidget->addWidget(d->mThumbnailPage);
	d->mCentralWidget->addWidget(d->mProgressPage);
	d->mCentralWidget->addWidget(d->mDialogPage);

	connect(d->mThumbnailPage, SIGNAL(importRequested()),
		SLOT(startImport()));
	connect(d->mThumbnailPage, SIGNAL(rejected()),
		SLOT(close()));
	connect(d->mImporter, SIGNAL(importFinished()),
		SLOT(slotImportFinished()));

	d->mCentralWidget->setCurrentWidget(d->mThumbnailPage);

	setAutoSaveSettings();
}


ImportDialog::~ImportDialog() {
	delete d;
}


QSize ImportDialog::sizeHint() const {
	return QSize(700, 500);
}


void ImportDialog::setSourceUrl(const KUrl& url) {
	d->mThumbnailPage->setSourceUrl(url);
}


void ImportDialog::startImport() {
	KUrl url = d->mThumbnailPage->destinationUrl();
	ImporterConfig::setDestinationUrl(url);
	ImporterConfig::self()->writeConfig();

	d->mCentralWidget->setCurrentWidget(d->mProgressPage);
	d->mImporter->start(d->mThumbnailPage->urlList(), url);
}


void ImportDialog::slotImportFinished() {
	d->deleteImportedUrls();
	d->showWhatNext();
}


void ImportDialog::showImportError(const QString& message) {
	KMessageBox::sorry(this, message);
	d->mCentralWidget->setCurrentWidget(d->mThumbnailPage);
}


} // namespace
