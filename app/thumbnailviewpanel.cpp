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
#include <lib/sorteddirmodel.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailview/thumbnailview.h>

namespace Gwenview {


struct ThumbnailViewPanelPrivate {
	ThumbnailViewPanel* that;
	ThumbnailView* mThumbnailView;
	KUrlNavigator* mUrlNavigator;
	QSlider* mThumbnailSlider;
	QWidget* mFilterBar;
	KLineEdit* mFilterEdit;
	SortedDirModel* mDirModel;
	KToggleAction* mShowFilterBar;

	void setupWidgets() {
		// mThumbnailView
		mThumbnailView = new ThumbnailView(that);
		mThumbnailView->setModel(mDirModel);

		PreviewItemDelegate* delegate = new PreviewItemDelegate(mThumbnailView);
		mThumbnailView->setItemDelegate(delegate);
		mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);

		// mUrlNavigator
		KFilePlacesModel* places = new KFilePlacesModel(that);
		mUrlNavigator = new KUrlNavigator(places, KUrl(), that);

		// Rating slider
		QSlider* ratingSlider = new QSlider;
		ratingSlider->setOrientation(Qt::Horizontal);
		ratingSlider->setMinimum(0);
		ratingSlider->setMaximum(5);
		QObject::connect(ratingSlider, SIGNAL(valueChanged(int)), mDirModel, SLOT(setMinimumRating(int)) );

		// Thumbnail slider
		mThumbnailSlider = new QSlider;
		mThumbnailSlider->setMaximumWidth(200);
		mThumbnailSlider->setMinimum(40);
		mThumbnailSlider->setMaximum(256);
		mThumbnailSlider->setOrientation(Qt::Horizontal);
		QObject::connect(mThumbnailSlider, SIGNAL(valueChanged(int)), mThumbnailView, SLOT(setThumbnailSize(int)) );

		// Status bar
		KStatusBar* statusBar = new KStatusBar(that);
		statusBar->addPermanentWidget(ratingSlider);
		statusBar->addPermanentWidget(mThumbnailSlider);

		setupFilterBar();

		// Layout
		QVBoxLayout* layout = new QVBoxLayout(that);
		layout->setSpacing(0);
		layout->setMargin(0);
		layout->addWidget(mUrlNavigator);
		layout->addWidget(mThumbnailView);
		layout->addWidget(mFilterBar);
		layout->addWidget(statusBar);
	}

	void setupFilterBar() {
		mFilterBar = new QWidget(that);
		mFilterBar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

		QTimer* timer = new QTimer(mFilterBar);
		timer->setInterval(350);
		timer->setSingleShot(true);
		QObject::connect(timer, SIGNAL(timeout()),
			that, SLOT(applyNameFilter()));

		mFilterEdit = new KLineEdit(mFilterBar);
		mFilterEdit->setClickMessage(i18n("Enter search terms here"));
		mFilterEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		mFilterEdit->setClearButtonShown(true);
		mFilterEdit->setToolTip(i18n("Enter space-separated terms to search in the thumbnail view."));
		QObject::connect(mFilterEdit, SIGNAL(textChanged(const QString &)),
			timer, SLOT(start()));

		QLabel* label = new QLabel(i18nc("@label:textbox", "Filter:"), mFilterBar);
		label->setBuddy(mFilterEdit);

		QToolButton* closeButton = new QToolButton(mFilterBar);
		closeButton->setAutoRaise(true);
		closeButton->setIcon(KIcon("dialog-close"));
		closeButton->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
		QObject::connect(closeButton, SIGNAL(clicked(bool)),
			that, SLOT(toggleFilterBarVisibility(bool)));

		QHBoxLayout* filterBoxLayout = new QHBoxLayout(mFilterBar);
		filterBoxLayout->addWidget(label);
		filterBoxLayout->addWidget(mFilterEdit);
		filterBoxLayout->addWidget(closeButton);
		mFilterBar->hide();
	}

	void setupActions(KActionCollection* actionCollection) {
		mShowFilterBar = actionCollection->add<KToggleAction>("toggle_filterbar");
		mShowFilterBar->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
		mShowFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
		QObject::connect(mShowFilterBar, SIGNAL(triggered(bool)),
			that, SLOT(toggleFilterBarVisibility(bool)));

		KAction* editLocationAction = actionCollection->addAction("edit_location");
		editLocationAction->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
		editLocationAction->setShortcut(Qt::Key_F6);
		QObject::connect(editLocationAction, SIGNAL(triggered()),
			that, SLOT(editLocation()));
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


void ThumbnailViewPanel::toggleFilterBarVisibility(bool value) {
	d->mFilterBar->setVisible(value);
	d->mShowFilterBar->setChecked(value);
	if (value) {
		d->mFilterEdit->setFocus();
	} else {
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
