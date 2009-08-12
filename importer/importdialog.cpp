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
#include <QStackedWidget>

// KDE
#include <kdebug.h>
#include <krun.h>
#include <kservice.h>

// Local
#include "progresspage.h"
#include "thumbnailpage.h"

namespace Gwenview {


class ImportDialogPrivate {
public:
	QStackedWidget* mCentralWidget;
	ThumbnailPage* mThumbnailPage;
	ProgressPage* mProgressPage;
};


ImportDialog::ImportDialog()
: d(new ImportDialogPrivate) {
	d->mThumbnailPage = new ThumbnailPage;
	d->mProgressPage = new ProgressPage;

	d->mCentralWidget = new QStackedWidget;
	setCentralWidget(d->mCentralWidget);
	d->mCentralWidget->addWidget(d->mThumbnailPage);
	d->mCentralWidget->addWidget(d->mProgressPage);

	connect(d->mThumbnailPage, SIGNAL(importClicked()),
		SLOT(startImport()));
	connect(d->mProgressPage, SIGNAL(importFinished()),
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
	d->mCentralWidget->setCurrentWidget(d->mProgressPage);
	d->mProgressPage->start(d->mThumbnailPage->urlList());
}


void ImportDialog::slotImportFinished() {
	KService::Ptr service = KService::serviceByDesktopName("gwenview");
	if (!service) {
		KRun::run(*service, KUrl::List() << d->mThumbnailPage->destinationUrl(), 0 /* window */);
	} else {
		kError() << "Could not find gwenview";
	}
	qApp->quit();
}


} // namespace

