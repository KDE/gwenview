// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "thumbnailpage.h"

// Qt
#include <QPushButton>

// KDE
#include <kdebug.h>
#include <kdirlister.h>

// Local
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <ui_thumbnailpage.h>

namespace Gwenview {


class ImporterThumbnailViewHelper : public AbstractThumbnailViewHelper {
public:
	ImporterThumbnailViewHelper(QObject* parent)
	: AbstractThumbnailViewHelper(parent)
	{}

	void showContextMenu(QWidget*)
	{}

	void showMenuForUrlDroppedOnViewport(QWidget*, const KUrl::List&)
	{}

	void showMenuForUrlDroppedOnDir(QWidget*, const KUrl::List&, const KUrl&)
	{}

	bool isDocumentModified(const KUrl&) {
		return false;
	}

	void thumbnailForDocument(const KUrl&, ThumbnailGroup::Enum, QPixmap* outPix, QSize* outFullSize) const {
		*outPix = QPixmap();
		*outFullSize = QSize();
	}
};


struct ThumbnailPagePrivate : public Ui_ThumbnailPage {
	ThumbnailPage* q;
	SortedDirModel* mDirModel;
	QPushButton* mImportButton;
	KUrl::List mUrlList;

	void setupButtonBox() {
		mImportButton = mButtonBox->addButton(
			i18n("Import Selected"), QDialogButtonBox::AcceptRole,
			q, SLOT(slotImportSelected()));
		mImportButton->setEnabled(false);

		mButtonBox->addButton(
			i18n("Import All"), QDialogButtonBox::AcceptRole,
			q, SLOT(slotImportAll()));
	}
};


ThumbnailPage::ThumbnailPage()
: d(new ThumbnailPagePrivate) {
	d->q = this;
	d->setupUi(this);
	d->mDirModel = new SortedDirModel(this);
	d->mThumbnailView->setModel(d->mDirModel);
	d->mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	connect(d->mThumbnailView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		SLOT(updateImportSelectedButton()));
	d->mThumbnailView->setThumbnailViewHelper(new ImporterThumbnailViewHelper(this));

	d->setupButtonBox();
}


ThumbnailPage::~ThumbnailPage() {
	delete d;
}


void ThumbnailPage::setSourceUrl(const KUrl& url) {
	d->mDirModel->dirLister()->openUrl(url);
}


KUrl::List ThumbnailPage::urlList() const {
	return d->mUrlList;
}


KUrl ThumbnailPage::destinationUrl() const {
	return KUrl();
}


void ThumbnailPage::slotImportSelected() {
	d->mUrlList.clear();
	QModelIndexList list = d->mThumbnailView->selectionModel()->selectedIndexes();
	Q_FOREACH(const QModelIndex& index, list) {
		d->mUrlList << d->mDirModel->urlForIndex(index);
	}
	emit importRequested();
}


void ThumbnailPage::slotImportAll() {
	d->mUrlList.clear();
	for (int row = d->mDirModel->rowCount() - 1; row >= 0; --row) {
		QModelIndex index = d->mDirModel->index(row, 0);
		d->mUrlList << d->mDirModel->urlForIndex(index);
	}
	emit importRequested();
}


void ThumbnailPage::updateImportSelectedButton() {
	d->mImportButton->setEnabled(d->mThumbnailView->selectionModel()->hasSelection());
}


} // namespace
