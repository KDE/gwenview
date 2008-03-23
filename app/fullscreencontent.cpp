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
#include "fullscreencontent.h"

// Qt
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QToolBar>

// KDE
#include "kaction.h"
#include "kactioncollection.h"

// Local
#include "thumbnailbarview.h"
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfomodel.h>
#include <lib/slideshow.h>

namespace Gwenview {


struct FullScreenContentPrivate {
	ThumbnailBarView* mThumbnailBar;
	QLabel* mInformationLabel;
	Document::Ptr mCurrentDocument;
};


FullScreenContent::FullScreenContent(QWidget* parent, KActionCollection* actionCollection, SlideShow* slideShow)
: QObject(parent)
, d(new FullScreenContentPrivate) {
	parent->installEventFilter(this);

	QToolBar* bar = new QToolBar;
	bar->setFloatable(false);
	bar->setIconSize(QSize(32, 32));
	bar->addAction(actionCollection->action("fullscreen"));
	bar->addAction(actionCollection->action("go_previous"));
	bar->addAction(actionCollection->action("go_next"));

	bar->addSeparator();
	bar->addAction(actionCollection->action("toggle_slideshow"));
	bar->addWidget(slideShow->intervalWidget());
	bar->addWidget(slideShow->optionsWidget());

	d->mThumbnailBar = new ThumbnailBarView(parent);
	ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
	d->mThumbnailBar->setItemDelegate(delegate);
	d->mThumbnailBar->setThumbnailSize(64);

	d->mInformationLabel = new QLabel;

	QGridLayout* layout = new QGridLayout(parent);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(bar, 0, 0);
	layout->addWidget(d->mInformationLabel, 1, 0);
	layout->addWidget(d->mThumbnailBar, 0, 1, 2, 1);
}


FullScreenContent::~FullScreenContent() {
	delete d;
}


ThumbnailBarView* FullScreenContent::thumbnailBar() const {
	return d->mThumbnailBar;
}


void FullScreenContent::setCurrentUrl(const KUrl& url) {
	d->mCurrentDocument = DocumentFactory::instance()->load(url);
	connect(d->mCurrentDocument.data(),SIGNAL(metaDataUpdated()),
		SLOT(updateInformationLabel()) );
	updateInformationLabel();
}


void FullScreenContent::updateInformationLabel() {
	if (!d->mCurrentDocument) {
		kWarning() << "No document";
		return;
	}

	if (!d->mInformationLabel->isVisible()) {
		return;
	}

	ImageMetaInfoModel* imageMetaInfo = d->mCurrentDocument->metaInfo();

	QString aperture, exposureTime, iso, focalLength;
	QString filename;

	aperture = imageMetaInfo->getValueForKey("Exif.Photo.FNumber");
	exposureTime = imageMetaInfo->getValueForKey("Exif.Photo.ExposureTime");
	iso = imageMetaInfo->getValueForKey("Exif.Photo.ISOSpeedRatings");
	focalLength = imageMetaInfo->getValueForKey("Exif.Photo.FocalLength");

	filename = imageMetaInfo->getValueForKey("General.Name");

	QString info = GwenviewConfig::fullScreenInfo();
	info.replace("%a", aperture);
	info.replace("%t", exposureTime);
	info.replace("%i", iso);
	info.replace("%l", focalLength);
	info.replace("%f", filename);

	d->mInformationLabel->setText(info);
}


bool FullScreenContent::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::Show) {
		updateInformationLabel();
	}
	return false;
}


} // namespace
