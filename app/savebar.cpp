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
#include <QHBoxLayout>
#include <QLabel>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "lib/documentfactory.h"

namespace Gwenview {


struct SaveBarPrivate {
	QLabel* mMessage;
	QLabel* mActions;

	void save() {
		kDebug() << "save\n";
	}

	void saveAll() {
		kDebug() << "save all\n";
	}
};


SaveBar::SaveBar(QWidget* parent)
: QWidget(parent)
, d(new SaveBarPrivate) {
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
	if (lst.size() == 1) {
		KUrl url = lst[0];
		d->mMessage->setText(i18n("One image modified: %1", url.pathOrUrl()) );
		links << "<a href='save'>Save</a>";
	} else {
		d->mMessage->setText(i18n("%1 images modified", lst.size()) );
		links << "<a href='saveAll'>Save All</a>";
	}


	d->mActions->setText(links.join(" | "));
}


void SaveBar::triggerAction(const QString& action) {
	if (action == "save") {
		d->save();
	} else if (action == "saveAll") {
		d->saveAll();
	} else {
		kWarning() << "Unknown action: " << action << endl;
	}
}


} // namespace
