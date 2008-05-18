// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QHBoxLayout>
#include <QLabel>
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
	QLabel* mMessageLabel;
	QLabel* mActionsLabel;
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
	d->mMessageLabel = new QLabel(d->mSaveBarWidget);
	d->mMessageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mActionsLabel = new QLabel(d->mSaveBarWidget);
	d->mActionsLabel->setAlignment(Qt::AlignRight);
	d->mActionsLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	d->mForceHide = false;

	QHBoxLayout* layout = new QHBoxLayout(d->mSaveBarWidget);
	layout->addWidget(d->mMessageLabel);
	layout->addWidget(d->mActionsLabel);
	layout->setMargin(0);
	layout->setSpacing(0);
	hide();

	setContent(d->mSaveBarWidget);

	connect(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()),
		SLOT(updateContent()) );

	connect(d->mActionsLabel, SIGNAL(linkActivated(const QString&)),
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


	d->mMessageLabel->setText(message);
	d->mActionsLabel->setText(links.join(" | "));
}


void SaveBar::triggerAction(const QString& action) {
	QList<KUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
	if (action == "save") {
		requestSave(d->mCurrentUrl);
	} else if (action == "saveAll") {
		requestSaveAll();
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
