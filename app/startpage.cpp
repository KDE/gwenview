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
#include <QStandardItemModel>

// KDE
#include <kfileplacesmodel.h>
#include <kicon.h>
#include <kmimetype.h>

// Local
#include <ui_startpage.h>
#include <lib/flowlayout.h>
#include <lib/gwenviewconfig.h>

#ifndef GWENVIEW_SEMANTICINFOBACKEND_NONE
#include <lib/semanticinfo/tagmodel.h>
#endif

namespace Gwenview {

struct StartPagePrivate : public Ui_StartPage{
	StartPage* that;
	KFilePlacesModel* mBookmarksModel;
	QStandardItemModel* mRecentFoldersModel;

	void updateRecentFoldersModel() {
		QStringList list = GwenviewConfig::recentFolders();

		mRecentFoldersModel->clear();
		Q_FOREACH(const QString& urlString, list) {
			KUrl url(urlString);

			QStandardItem* item = new QStandardItem;
			item->setText(url.pathOrUrl());

			QString iconName = KMimeType::iconNameForUrl(url);
			item->setIcon(KIcon(iconName));

			item->setData(QVariant(url), KFilePlacesModel::UrlRole);

			mRecentFoldersModel->appendRow(item);
		}
	}

	void setupSearchUi(AbstractSemanticInfoBackEnd* backEnd) {
	#ifdef GWENVIEW_SEMANTICINFOBACKEND_NONE
		mTagLabel->setText(i18n(
			"Sorry, browsing by tag is not available. Make sure Nepomuk is properly installed on your computer."
			));
		mTagView->hide();
		mTagLabel->show();
	#else
		mTagView->setModel(TagModel::createAllTagsModel(0, backEnd));
		mTagView->show();
		mTagLabel->hide();
	#endif
	}
};


StartPage::StartPage(QWidget* parent, AbstractSemanticInfoBackEnd* backEnd)
: QFrame(parent)
, d(new StartPagePrivate) {
	d->that = this;
	d->setupUi(this);

	d->mBookmarksModel = new KFilePlacesModel(this);
	d->mRecentFoldersModel = new QStandardItemModel(this);

	d->mBookmarksView->setModel(d->mBookmarksModel);
	d->mBookmarksView->setAutoResizeItemsEnabled(false);
	d->mRecentFoldersView->setModel(d->mRecentFoldersModel);

	connect(d->mBookmarksView, SIGNAL(urlChanged(const KUrl&)),
		SIGNAL(urlSelected(const KUrl&)) );

	connect(d->mRecentFoldersView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotListViewClicked(const QModelIndex&)) );

	connect(d->mTagView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotTagViewClicked(const QModelIndex&)));

	d->setupSearchUi(backEnd);
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
	d->updateRecentFoldersModel();
	QFrame::showEvent(event);
}


} // namespace
