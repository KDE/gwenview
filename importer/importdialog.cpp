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
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kservice.h>
#include <kstandardguiitem.h>

// Local
#include "importer.h"
#include "importerconfig.h"
#include "progresspage.h"
#include "thumbnailpage.h"

namespace Gwenview {


class ImportDialogPrivate {
public:
	QStackedWidget* mCentralWidget;
	ThumbnailPage* mThumbnailPage;
	ProgressPage* mProgressPage;
	Importer* mImporter;

	void deleteImportedUrls() {
		int count = mImporter->importedUrlCount();
		if (count == 0) {
			return;
		}
		int answer = KMessageBox::questionYesNo(mCentralWidget,
			i18np(
				"One document has been successfully imported.\nDelete it from the device?",
				"%1 documents has been successfully imported.\nDelete them from the device?",
				count),
			QString(),
			KStandardGuiItem::del(),
			KGuiItem(i18n("Keep"))
			);
		if (answer != KMessageBox::Yes) {
			return;
		}
		mImporter->deleteImportedUrls();
	}

	void startGwenview() {
		KService::Ptr service = KService::serviceByDesktopName("gwenview");
		if (!service) {
			kError() << "Could not find gwenview";
		} else {
			KRun::run(*service, KUrl::List() << mThumbnailPage->destinationUrl(), 0 /* window */);
		}
	}
};


ImportDialog::ImportDialog()
: d(new ImportDialogPrivate) {
	d->mImporter = new Importer(this);
	d->mThumbnailPage = new ThumbnailPage;

	KUrl url = ImporterConfig::destinationUrl();
	if (!url.isValid()) {
		url = KUrl::fromPath(KGlobalSettings::picturesPath());
		int year = QDate::currentDate().year();
		url.addPath(QString::number(year));
	}
	d->mThumbnailPage->setDestinationUrl(url);

	d->mProgressPage = new ProgressPage(d->mImporter);

	d->mCentralWidget = new QStackedWidget;
	setCentralWidget(d->mCentralWidget);
	d->mCentralWidget->addWidget(d->mThumbnailPage);
	d->mCentralWidget->addWidget(d->mProgressPage);

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
	d->startGwenview();
	qApp->quit();
}


} // namespace
