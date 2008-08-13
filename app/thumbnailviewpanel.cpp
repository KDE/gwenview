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
#include <QSlider>
#include <QVBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kfileplacesmodel.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <ktoggleaction.h>
#include <kurlnavigator.h>

// Local
#include <lib/gwenviewconfig.h>
#include <lib/metadata/abstractmetadatabackend.h>
#include <lib/metadata/sorteddirmodel.h>
#include <lib/metadata/tagmodel.h>
#include <lib/archiveutils.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <ui_thumbnailviewpanel.h>

namespace Gwenview {


struct ThumbnailViewPanelPrivate : public Ui_ThumbnailViewPanel {
	ThumbnailViewPanel* that;
	KFilePlacesModel* mFilePlacesModel;
	KUrlNavigator* mUrlNavigator;
	SortedDirModel* mDirModel;
	int mDocumentCount;
	KActionCollection* mActionCollection;

	void setupWidgets() {
		setupUi(that);
		that->layout()->setMargin(0);

		// mThumbnailView
		mThumbnailView->setModel(mDirModel);

		PreviewItemDelegate* delegate = new PreviewItemDelegate(mThumbnailView);
		mThumbnailView->setItemDelegate(delegate);
		mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);

		// mUrlNavigator (use stupid layouting code because KUrlNavigator ctor
		// can't be used directly from Designer)
		mFilePlacesModel = new KFilePlacesModel(that);
		mUrlNavigator = new KUrlNavigator(mFilePlacesModel, KUrl(), mUrlNavigatorContainer);
		QVBoxLayout* layout = new QVBoxLayout(mUrlNavigatorContainer);
		layout->setMargin(0);
		layout->addWidget(mUrlNavigator);

		// StatusBar container, a standard widget with QStatusBar-like margins
		// (see QStatusBar::reformat())
		mStatusBarContainer->layout()->setContentsMargins(2, 3, 2, 2);

		// Thumbnail slider
		QObject::connect(mThumbnailSlider, SIGNAL(valueChanged(int)),
			mThumbnailView, SLOT(setThumbnailSize(int)) );

		// Filter widget
		mFilterWidget->setDirModel(mDirModel);
	}

	void setupActions(KActionCollection* actionCollection) {
		KAction* editLocationAction = actionCollection->addAction("edit_location");
		editLocationAction->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
		editLocationAction->setShortcut(Qt::Key_F6);
		QObject::connect(editLocationAction, SIGNAL(triggered()),
			that, SLOT(editLocation()));

		KAction* action = actionCollection->addAction("add_folder_to_places");
		action->setText(i18nc("@action:inmenu", "Add Folder to Places"));
		QObject::connect(action, SIGNAL(triggered()),
			that, SLOT(addFolderToPlaces()));
	}

	void updateDocumentCountLabel() {
		QString text = i18ncp("@label", "%1 document", "%1 documents", mDocumentCount);
		mDocumentCountLabel->setText(text);
	}

	void setupDocumentCountConnections() {
		QObject::connect(mDirModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
			that, SLOT(slotDirModelRowsInserted(const QModelIndex&, int, int)) );

		QObject::connect(mDirModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
			that, SLOT(slotDirModelRowsAboutToBeRemoved(const QModelIndex&, int, int)) );

		QObject::connect(mDirModel, SIGNAL(modelReset()),
			that, SLOT(slotDirModelReset()) );
	}


	int documentCountInIndexRange(const QModelIndex& parent, int start, int end) {
		int count = 0;
		for (int row=start; row<=end; ++row) {
			QModelIndex index = mDirModel->index(row, 0, parent);
			KFileItem item = mDirModel->itemForIndex(index);
			if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
				++count;
			}
		}
		return count;
	}

	void loadConfig() {
		mUrlNavigator->setUrlEditable(GwenviewConfig::urlNavigatorIsEditable());
		mUrlNavigator->setShowFullPath(GwenviewConfig::urlNavigatorShowFullPath());
	}

	void saveConfig() {
		GwenviewConfig::setUrlNavigatorIsEditable(mUrlNavigator->isUrlEditable());
		GwenviewConfig::setUrlNavigatorShowFullPath(mUrlNavigator->showFullPath());
	}
};


ThumbnailViewPanel::ThumbnailViewPanel(QWidget* parent, SortedDirModel* dirModel, KActionCollection* actionCollection)
: QWidget(parent)
, d(new ThumbnailViewPanelPrivate) {
	d->that = this;
	d->mDirModel = dirModel;
	d->mDocumentCount = 0;
	d->mActionCollection = actionCollection;
	d->setupWidgets();
	d->setupActions(actionCollection);
	d->setupDocumentCountConnections();
	d->loadConfig();
}


ThumbnailViewPanel::~ThumbnailViewPanel() {
	d->saveConfig();
	delete d;
}


void ThumbnailViewPanel::initActionDependentWidgets() {
	d->mStartPageButton->setDefaultAction(d->mActionCollection->action("go_start_page"));
}


void ThumbnailViewPanel::setStatusBarHeight(int height) {
	d->mStatusBarContainer->setFixedHeight(height);
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


void ThumbnailViewPanel::editLocation() {
	d->mUrlNavigator->setUrlEditable(true);
	d->mUrlNavigator->setFocus();
}


void ThumbnailViewPanel::addFolderToPlaces() {
	KUrl url = d->mUrlNavigator->url();
	QString text = url.fileName();
	if (text.isEmpty()) {
		text = url.pathOrUrl();
	}
	d->mFilePlacesModel->addPlace(text, url);
}


void ThumbnailViewPanel::slotDirModelRowsInserted(const QModelIndex& parent, int start, int end) {
	int count = d->documentCountInIndexRange(parent, start, end);
	if (count > 0) {
		d->mDocumentCount += count;
		d->updateDocumentCountLabel();
	}
}


void ThumbnailViewPanel::slotDirModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
	int count = d->documentCountInIndexRange(parent, start, end);
	if (count > 0) {
		d->mDocumentCount -= count;
		d->updateDocumentCountLabel();
	}
}


void ThumbnailViewPanel::slotDirModelReset() {
	d->mDocumentCount = 0;
	d->updateDocumentCountLabel();
}


} // namespace
