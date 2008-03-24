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
#include <QDoubleSpinBox>
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QToolBar>
#include <QToolButton>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>

// Local
#include "imagemetainfodialog.h"
#include "thumbnailbarview.h"
#include <lib/document/document.h>
#include <lib/document/documentfactory.h>
#include <lib/gwenviewconfig.h>
#include <lib/imagemetainfomodel.h>
#include <lib/slideshow.h>

namespace Gwenview {


struct FullScreenContentPrivate {
	FullScreenContent* that;
	ThumbnailBarView* mThumbnailBar;
	QLabel* mInformationLabel;
	Document::Ptr mCurrentDocument;
	QPointer<ImageMetaInfoDialog> mImageMetaInfoDialog;

	QWidget* createOptionsButton(SlideShow* slideShow) {
		QToolButton* button = new QToolButton;
		button->setIcon(KIcon("configure"));
		button->setToolTip(i18nc("@info:tooltip", "Slideshow options"));
		QMenu* menu = new QMenu(button);
		button->setMenu(menu);
		button->setPopupMode(QToolButton::InstantPopup);
		menu->addAction(slideShow->loopAction());
		menu->addAction(slideShow->randomAction());

		menu->addSeparator();

		QAction* action = menu->addAction(i18nc("@menu:item", "Configure displayed information"));
		QObject::connect(action, SIGNAL(triggered()),
			that, SLOT(configureInformationLabel()) );

		return button;
	}


	QWidget* createSlideShowIntervalWidget(SlideShow* slideShow) {
		QDoubleSpinBox* spinBox = new QDoubleSpinBox;
		spinBox->setSuffix(i18nc("@item:intext spinbox suffix for slideshow interval", " seconds"));
		spinBox->setMinimum(0.5);
		spinBox->setMaximum(999999.);
		spinBox->setDecimals(1);
		spinBox->setValue(GwenviewConfig::interval());
		QObject::connect(spinBox, SIGNAL(valueChanged(double)),
			slideShow, SLOT(setInterval(double)) );

		return spinBox;
	}
};


FullScreenContent::FullScreenContent(QWidget* parent, KActionCollection* actionCollection, SlideShow* slideShow)
: QObject(parent)
, d(new FullScreenContentPrivate) {
	d->that = this;
	parent->installEventFilter(this);

	QWidget* optionsButton = d->createOptionsButton(slideShow);
	QWidget* slideShowIntervalWidget = d->createSlideShowIntervalWidget(slideShow);

	QToolBar* bar = new QToolBar;
	bar->setFloatable(false);
	bar->setIconSize(QSize(32, 32));
	bar->addAction(actionCollection->action("fullscreen"));
	bar->addAction(actionCollection->action("go_previous"));
	bar->addAction(actionCollection->action("go_next"));

	bar->addSeparator();
	bar->addAction(actionCollection->action("toggle_slideshow"));
	bar->addWidget(slideShowIntervalWidget);
	bar->addWidget(optionsButton);

	d->mThumbnailBar = new ThumbnailBarView(parent);
	ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(d->mThumbnailBar);
	d->mThumbnailBar->setItemDelegate(delegate);
	d->mThumbnailBar->setThumbnailSize(64);

	d->mInformationLabel = new QLabel;
	d->mInformationLabel->setWordWrap(true);

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
	connect(d->mCurrentDocument.data(),SIGNAL(metaDataUpdated()),
		SLOT(updateMetaInfoDialog()) );
	updateInformationLabel();
	updateMetaInfoDialog();
}


void FullScreenContent::updateInformationLabel() {
	if (!d->mCurrentDocument) {
		kWarning() << "No document";
		return;
	}

	if (!d->mInformationLabel->isVisible()) {
		return;
	}

	ImageMetaInfoModel* model = d->mCurrentDocument->metaInfo();

	QStringList valueList;
	Q_FOREACH(const QString& key, GwenviewConfig::fullScreenPreferredMetaInfoKeyList()) {
		valueList << model->getValueForKey(key);
	}
	QString text = valueList.join(i18nc("@item:intext fullscreen meta info separator", ", "));

	d->mInformationLabel->setText(text);
}


bool FullScreenContent::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::Show) {
		updateInformationLabel();
		updateMetaInfoDialog();
	}
	return false;
}


void FullScreenContent::configureInformationLabel() {
	if (!d->mImageMetaInfoDialog) {
		d->mImageMetaInfoDialog = new ImageMetaInfoDialog(d->mInformationLabel);
		d->mImageMetaInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(d->mImageMetaInfoDialog, SIGNAL(preferredMetaInfoKeyListChanged(const QStringList&)),
			SLOT(slotPreferredMetaInfoKeyListChanged(const QStringList&)) );
	}
	d->mImageMetaInfoDialog->setMetaInfo(d->mCurrentDocument->metaInfo(), GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
	d->mImageMetaInfoDialog->show();
}


void FullScreenContent::slotPreferredMetaInfoKeyListChanged(const QStringList& list) {
	GwenviewConfig::setFullScreenPreferredMetaInfoKeyList(list);
	GwenviewConfig::self()->writeConfig();
	updateInformationLabel();
}


void FullScreenContent::updateMetaInfoDialog() {
	if (!d->mImageMetaInfoDialog) {
		return;
	}
	ImageMetaInfoModel* model = d->mCurrentDocument ? d->mCurrentDocument->metaInfo() : 0;
	d->mImageMetaInfoDialog->setMetaInfo(model, GwenviewConfig::fullScreenPreferredMetaInfoKeyList());
}


} // namespace
