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
#include "thumbnailviewpanel.moc"

// Qt
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kfileplacesmodel.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <ktoggleaction.h>
#include <kurlnavigator.h>

// Local
#include <lib/metadata/sorteddirmodel.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <ui_thumbnailviewpanel.h>

namespace Gwenview {


struct ThumbnailViewPanelPrivate : public Ui_ThumbnailViewPanel {
	ThumbnailViewPanel* that;
	KUrlNavigator* mUrlNavigator;
	SortedDirModel* mDirModel;
	QAction* mShowFilterBar;

	void setupWidgets() {
		setupUi(that);
		that->layout()->setMargin(0);

		// mThumbnailView
		mThumbnailView->setModel(mDirModel);

		PreviewItemDelegate* delegate = new PreviewItemDelegate(mThumbnailView);
		mThumbnailView->setItemDelegate(delegate);
		mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);

		// mUrlNavigator
		KFilePlacesModel* places = new KFilePlacesModel(that);
		mUrlNavigator = new KUrlNavigator(places, KUrl(), that);
		static_cast<QVBoxLayout*>(that->layout())->insertWidget(0, mUrlNavigator);

		// Rating slider
		mRatingSlider->setOrientation(Qt::Horizontal);
		mRatingSlider->setMinimum(0);
		mRatingSlider->setMaximum(5);
		QObject::connect(mRatingSlider, SIGNAL(valueChanged(int)),
			mDirModel, SLOT(setMinimumRating(int)) );

		// Thumbnail slider
		QObject::connect(mThumbnailSlider, SIGNAL(valueChanged(int)),
			mThumbnailView, SLOT(setThumbnailSize(int)) );

		// Filter bar
		QTimer* timer = new QTimer(mFilterBar);
		timer->setInterval(350);
		timer->setSingleShot(true);
		QObject::connect(timer, SIGNAL(timeout()),
			that, SLOT(applyNameFilter()));

		QObject::connect(mFilterEdit, SIGNAL(textChanged(const QString &)),
			timer, SLOT(start()));

		mFilterBar->hide();
	}

	void setupActions(KActionCollection* actionCollection) {
		mShowFilterBar = actionCollection->addAction("toggle_filterbar");
		mShowFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
		QObject::connect(mShowFilterBar, SIGNAL(triggered()),
			that, SLOT(toggleFilterBarVisibility()));
		updateShowFilterBarAction();

		mFilterBarButton->setDefaultAction(mShowFilterBar);

		KAction* editLocationAction = actionCollection->addAction("edit_location");
		editLocationAction->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
		editLocationAction->setShortcut(Qt::Key_F6);
		QObject::connect(editLocationAction, SIGNAL(triggered()),
			that, SLOT(editLocation()));
	}

	void updateShowFilterBarAction() {
		QString text;
		if (mFilterBar->isVisible()) {
			text = i18nc("@action:inmenu Tools", "Hide Filter Bar");
		} else {
			text = i18nc("@action:inmenu Tools", "Show Filter Bar");
		}
		mShowFilterBar->setText(text);
	}
};


ThumbnailViewPanel::ThumbnailViewPanel(QWidget* parent, SortedDirModel* dirModel, KActionCollection* actionCollection)
: QWidget(parent)
, d(new ThumbnailViewPanelPrivate) {
	d->that = this;
	d->mDirModel = dirModel;
	d->setupWidgets();
	d->setupActions(actionCollection);
}


ThumbnailViewPanel::~ThumbnailViewPanel() {
	delete d;
}


ThumbnailView* ThumbnailViewPanel::thumbnailView() const {
	return d->mThumbnailView;
}


QSlider* ThumbnailViewPanel::thumbnailSlider() const {
	return d->mThumbnailSlider;
}


KUrlNavigator* ThumbnailViewPanel::urlNavigator() const {
	return d->mUrlNavigator;
}


void ThumbnailViewPanel::toggleFilterBarVisibility() {
	bool visible = !d->mFilterBar->isVisible();
	d->mFilterBar->setVisible(visible);
	d->updateShowFilterBarAction();
	if (visible) {
		d->mRatingSlider->setFocus();
	} else {
		d->mRatingSlider->setValue(0);
		d->mFilterEdit->clear();
	}
}


void ThumbnailViewPanel::applyNameFilter() {
	d->mDirModel->setFilterRegExp(d->mFilterEdit->text());
}


void ThumbnailViewPanel::editLocation() {
	d->mUrlNavigator->setUrlEditable(true);
	d->mUrlNavigator->setFocus();
}


} // namespace
