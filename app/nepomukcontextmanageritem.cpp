// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "nepomukcontextmanageritem.moc"

// Qt

// KDE
#include <klocale.h>

// Nepomuk
#include <nepomuk/global.h>
#include <nepomuk/kmetadatatagwidget.h>
#include <nepomuk/kratingwidget.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>

#include <Soprano/Vocabulary/Xesam>

// Local
#include "contextmanager.h"
#include "sidebar.h"
#include "ui_nepomuksidebaritem.h"

namespace Gwenview {


struct NepomukContextManagerItemPrivate : public Ui_NepomukSideBarItem {
	SideBar* mSideBar;
	SideBarGroup* mGroup;
};


NepomukContextManagerItem::NepomukContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new NepomukContextManagerItemPrivate) {
	d->mSideBar = 0;
	d->mGroup = 0;

	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );
	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateSideBarContent()) );
}


NepomukContextManagerItem::~NepomukContextManagerItem() {
	delete d;
}


void NepomukContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );

	d->mGroup = sideBar->createGroup(i18n("Meta Information"));

	QWidget* container = new QWidget;
	d->setupUi(container);
	container->layout()->setMargin(0);
	d->mGroup->addWidget(container);

	d->mRatingWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	connect(d->mRatingWidget, SIGNAL(ratingChanged(int)),
		SLOT(slotRatingChanged(int)));

	connect(d->mDescriptionLineEdit, SIGNAL(editingFinished()),
		SLOT(storeDescription()));
}


void NepomukContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar->isVisible()) {
		return;
	}

	QList<Nepomuk::Resource> resourceList;
	KFileItemList itemList = contextManager()->selection();

	bool first = true;
	int rating = 0;
	QString description;
	Q_FOREACH(const KFileItem& item, itemList) {
		QString urlString = item.url().url();
		Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());
		resourceList << resource;

		if (first) {
			rating = resource.rating();
		} else if (rating != (int)resource.rating()) {
			// Ratings aren't the same, reset
			rating = 0;
		}

		if (first) {
			description = resource.description();
		} else if (description != resource.description()) {
			description = QString();
		}

		first = false;
	}
	d->mRatingWidget->setRating(rating);
	d->mDescriptionLineEdit->setText(description);
	d->mTagWidget->setTaggedResources(resourceList);
}


void NepomukContextManagerItem::slotRatingChanged(int rating) {
	QList<Nepomuk::Resource> resourceList = d->mTagWidget->taggedResources();
	Q_FOREACH(Nepomuk::Resource resource, resourceList) {
		resource.setRating(rating);
	}
}


void NepomukContextManagerItem::storeDescription() {
	QString description = d->mDescriptionLineEdit->text();
	QList<Nepomuk::Resource> resourceList = d->mTagWidget->taggedResources();
	Q_FOREACH(Nepomuk::Resource resource, resourceList) {
		resource.setDescription(description);
	}
}


} // namespace
