// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "savebar.moc"

// Qt
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressDialog>
#include <QToolTip>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "lib/documentfactory.h"

namespace Gwenview {


struct SaveBarPrivate {
	QWidget* mSaveBar;
	QLabel* mMessage;
	QLabel* mActions;
	KUrl mCurrentUrl;

	void saveAll() {
		QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();

		// TODO: Save in a separate thread?
		QProgressDialog progress(mSaveBar);
		progress.setLabelText(i18n("Saving..."));
		progress.setCancelButtonText(i18n("&Stop"));
		progress.setMinimum(0);
		progress.setMinimum(lst.size());
		progress.setWindowModality(Qt::WindowModal);

		Q_FOREACH(KUrl url, lst) {
			Document::Ptr doc = DocumentFactory::instance()->load(url);
			doc->save(url, doc->format());
			progress.setValue(progress.value() + 1);
			if (progress.wasCanceled()) {
				return;
			}
			qApp->processEvents();
		}
	}
};


SaveBar::SaveBar(QWidget* parent)
: QWidget(parent)
, d(new SaveBarPrivate) {
	setPalette(QToolTip::palette());
	setAutoFillBackground(true);
	d->mSaveBar = this;
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mMessage = new QLabel(this);
	d->mMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mActions = new QLabel(this);
	d->mActions->setAlignment(Qt::AlignRight);
	d->mActions->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(d->mMessage);
	layout->addWidget(d->mActions);
	layout->setMargin(0);
	layout->setSpacing(0);
	hide();

	connect(DocumentFactory::instance(), SIGNAL(saved(const KUrl&)),
		SLOT(updateContent()) );
	connect(DocumentFactory::instance(), SIGNAL(modified(const KUrl&)),
		SLOT(updateContent()) );

	connect(d->mActions, SIGNAL(linkActivated(const QString&)),
		SLOT(triggerAction(const QString&)) );
}


SaveBar::~SaveBar() {
	delete d;
}


void SaveBar::updateContent() {
	QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
	if (lst.size() == 0) {
		hide();
		return;
	}

	show();

	QStringList links;
	QString message;

	if (lst.contains(d->mCurrentUrl)) {
		message = i18n("Current image modified");

		if (lst.size() > 1) {
			QString previous = i18n("Previous modified image");
			QString next = i18n("Next modified image");
			if (d->mCurrentUrl == lst[0]) {
				links << previous;
			} else {
				links << QString("<a href='previous'>%1</a>").arg(previous);
			}
			if (d->mCurrentUrl == lst[lst.size() - 1]) {
				links << next;
			} else {
				links << QString("<a href='next'>%1</a>").arg(next);
			}
		}
	} else {
		message = i18np("One image modified", "%1 images modified", lst.size());
		if (lst.size() > 1) {
			links << QString("<a href='first'>%1</a>").arg(i18n("Go to first modified image"));
		} else {
			links << QString("<a href='first'>%1</a>").arg(i18n("Go to it"));
		}
	}

	if (lst.contains(d->mCurrentUrl)) {
		links << QString("<a href='save'>%1</a>").arg(i18n("Save"));
	}
	if (lst.size() > 1) {
		links << QString("<a href='saveAll'>%1</a>").arg(i18n("Save All"));
	}


	d->mMessage->setText(message);
	d->mActions->setText(links.join(" | "));
}


void SaveBar::triggerAction(const QString& action) {
	QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
	if (action == "save") {
		requestSave(d->mCurrentUrl);
	} else if (action == "saveAll") {
		d->saveAll();
	} else if (action == "first") {
		goToUrl(lst[0]);
	} else if (action == "previous") {
		int pos = lst.indexOf(d->mCurrentUrl);
		--pos;
		Q_ASSERT(pos >= 0);
		goToUrl(lst[pos]);
	} else if (action == "next") {
		int pos = lst.indexOf(d->mCurrentUrl);
		++pos;
		Q_ASSERT(pos < lst.size());
		goToUrl(lst[pos]);
	} else {
		kWarning() << "Unknown action: " << action ;
	}
}


void SaveBar::setCurrentUrl(const KUrl& url) {
	d->mCurrentUrl = url;
	updateContent();
}


} // namespace
