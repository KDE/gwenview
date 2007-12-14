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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

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
#include <kurl.h>

// Local
#include "lib/document/documentfactory.h"
#include "lib/paintutils.h"


namespace Gwenview {


struct SaveBarPrivate {
	QWidget* mSaveBarWidget;
	QLabel* mMessage;
	QLabel* mActions;
	KUrl mCurrentUrl;
	bool mForceHide;

	void initBackground(QWidget* widget) {
		widget->setAutoFillBackground(true);

		QColor color = QToolTip::palette().base().color();
		QColor borderColor = PaintUtils::adjustedHsv(color, 0, 150, 0);

		QString css =
			"QWidget {"
			"	background-color: %1;"
			"	border-top: 1px solid %2;"
			"	border-bottom: 1px solid %2;"
			"	padding-top: 3px;"
			"	padding-bottom: 3px;"
			"}";
		css = css
			.arg(color.name())
			.arg(borderColor.name());
		widget->setStyleSheet(css);
	}
};


SaveBar::SaveBar(QWidget* parent)
: SlideContainer(parent)
, d(new SaveBarPrivate) {
	d->mSaveBarWidget = new QWidget();
	d->initBackground(d->mSaveBarWidget);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mMessage = new QLabel(d->mSaveBarWidget);
	d->mMessage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mActions = new QLabel(d->mSaveBarWidget);
	d->mActions->setAlignment(Qt::AlignRight);
	d->mActions->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	d->mForceHide = false;

	QHBoxLayout* layout = new QHBoxLayout(d->mSaveBarWidget);
	layout->addWidget(d->mMessage);
	layout->addWidget(d->mActions);
	layout->setMargin(0);
	layout->setSpacing(0);
	hide();

	setContent(d->mSaveBarWidget);

	connect(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()),
		SLOT(updateContent()) );

	connect(d->mActions, SIGNAL(linkActivated(const QString&)),
		SLOT(triggerAction(const QString&)) );
}


SaveBar::~SaveBar() {
	delete d;
}


void SaveBar::setForceHide(bool value) {
	d->mForceHide = value;
	if (d->mForceHide) {
		slideOut();
	} else {
		updateContent();
	}
}


void SaveBar::updateContent() {
	if (d->mForceHide) {
		slideOut();
		return;
	}

	QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
	if (lst.size() == 0) {
		slideOut();
		return;
	}

	slideIn();

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
		saveAll();
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


bool SaveBar::saveAll() {
	QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();

	// TODO: Save in a separate thread?
	QProgressDialog progress(this);
	progress.setLabelText(i18n("Saving..."));
	progress.setCancelButtonText(i18n("&Stop"));
	progress.setMinimum(0);
	progress.setMinimum(lst.size());
	progress.setWindowModality(Qt::WindowModal);

	Q_FOREACH(KUrl url, lst) {
		Document::Ptr doc = DocumentFactory::instance()->load(url);
		Document::SaveResult saveResult = doc->save(url, doc->format());
		if (saveResult != Document::SR_OK) {
			// FIXME: Message
			return false;
		}
		progress.setValue(progress.value() + 1);
		if (progress.wasCanceled()) {
			return false;
		}
		qApp->processEvents();
	}

	return true;
}


} // namespace
