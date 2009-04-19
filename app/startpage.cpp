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
#include "startpage.moc"

#include "config-gwenview.h"

// Qt
#include <QListView>
#include <QMenu>

// KDE
#include <kfileplacesmodel.h>
#include <kicon.h>
#include <kmimetype.h>

// Local
#include <gvcore.h>
#include <ui_startpage.h>
#include <lib/flowlayout.h>
#include <lib/gwenviewconfig.h>

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <lib/semanticinfo/tagmodel.h>
#endif

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK
#include <nepomuk/resourcemanager.h>
#endif


namespace Gwenview {

struct StartPagePrivate : public Ui_StartPage{
	StartPage* that;
	GvCore* mGvCore;
	KFilePlacesModel* mBookmarksModel;

	void setupSearchUi(AbstractSemanticInfoBackEnd* backEnd) {
	#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK
		if (Nepomuk::ResourceManager::instance()->init() == 0) {
			mTagView->setModel(TagModel::createAllTagsModel(mTagView, backEnd));
			mTagView->show();
			mTagLabel->hide();
		} else {
			mTagLabel->setText(i18n(
				"Sorry, browsing by tag is not available. Make sure Nepomuk is properly installed on your computer."
				));
			mTagView->hide();
			mTagLabel->show();
		}
	#else
		mTagView->hide();
		mTagTitleLabel->hide();
	#endif
	}
};


StartPage::StartPage(QWidget* parent, GvCore* gvCore)
: QFrame(parent)
, d(new StartPagePrivate) {
	d->that = this;
	d->mGvCore = gvCore;
	d->setupUi(this);
	setFrameStyle(QFrame::NoFrame);

	d->mBookmarksModel = new KFilePlacesModel(this);

	d->mBookmarksView->setModel(d->mBookmarksModel);
	d->mBookmarksView->setAutoResizeItemsEnabled(false);

	connect(d->mBookmarksView, SIGNAL(urlChanged(const KUrl&)),
		SIGNAL(urlSelected(const KUrl&)) );

	connect(d->mRecentFoldersView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotListViewClicked(const QModelIndex&)) );

	connect(d->mRecentFoldersView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showRecentFoldersViewContextMenu(const QPoint&)));

	connect(d->mTagView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotTagViewClicked(const QModelIndex&)));

	d->setupSearchUi(gvCore->semanticInfoBackEnd());
}


StartPage::~StartPage() {
	delete d;
}


void StartPage::slotTagViewClicked(const QModelIndex& index) {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK
	if (!index.isValid()) {
		return;
	}
	// FIXME: Check label encoding
	KUrl url("nepomuksearch:/hasTag:" + index.data().toString());
	emit urlSelected(url);
#endif
}


void StartPage::applyPalette(const QPalette& newPalette) {
	QColor fgColor = newPalette.text().color();

	QPalette pal = palette();
	pal.setBrush(backgroundRole(), newPalette.base());
	setPalette(pal);

	QString css = QString::fromUtf8(
		"#StartPage > QLabel, #StartPage > QListView {"
		"	color: %1;"
		"}"

		"#StartPage > QLabel[title=true] {"
		"	font-weight: bold;"
		"	border-bottom: 1px solid %1;"
		"}"

		"#StartPage > QListView {"
		"	background-color: transparent;"
		"}"
		)
		.arg(fgColor.name());
	setStyleSheet(css);
}


void StartPage::slotListViewClicked(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}
	QVariant data = index.data(KFilePlacesModel::UrlRole);
	KUrl url = data.toUrl();

	// Prevent dir lister error
	if (!url.isValid()) {
		kError() << "Tried to open an invalid url";
		return;
	}
	emit urlSelected(url);
}


void StartPage::showEvent(QShowEvent* event) {
	if (!d->mRecentFoldersView->model()) {
		d->mRecentFoldersView->setModel(d->mGvCore->recentFoldersModel());
	}
	QFrame::showEvent(event);
}


void StartPage::showRecentFoldersViewContextMenu(const QPoint& pos) {
	QModelIndex index = d->mRecentFoldersView->indexAt(pos);
	if (!index.isValid()) {
		return;
	}
	QVariant data = index.data(KFilePlacesModel::UrlRole);
	KUrl url = data.toUrl();

	QMenu menu(this);
	QAction* addToPlacesAction = menu.addAction(KIcon("bookmark-new"), i18n("Add to Places"));
	QAction* removeAction = menu.addAction(KIcon("edit-delete"), i18n("Forget this Folder"));
	QAction* action = menu.exec(d->mRecentFoldersView->mapToGlobal(pos));
	if (action == addToPlacesAction) {
		QString text = url.fileName();
		if (text.isEmpty()) {
			text = url.pathOrUrl();
		}
		d->mBookmarksModel->addPlace(text, url);
	} else if (action == removeAction) {
		//d->mGvCore->recentFoldersModel()->removeUrl(url);
	}
}


} // namespace
