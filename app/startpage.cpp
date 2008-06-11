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

// Qt
#include <QListView>
#include <QStandardItemModel>

// KDE
#include <kfileplacesmodel.h>
#include <kicon.h>
#include <kmimetype.h>

// Local
#include <ui_startpage.h>
#include <lib/gwenviewconfig.h>

namespace Gwenview {


struct StartPagePrivate : public Ui_StartPage{
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
};


StartPage::StartPage(QWidget* parent)
: QFrame(parent)
, d(new StartPagePrivate) {
	d->setupUi(this);

	d->mBookmarksModel = new KFilePlacesModel(this);
	d->mRecentFoldersModel = new QStandardItemModel(this);

	d->mBookmarksView->setModel(d->mBookmarksModel);
	d->mRecentFoldersView->setModel(d->mRecentFoldersModel);

	connect(d->mBookmarksView, SIGNAL(urlChanged(const KUrl&)),
		SIGNAL(urlSelected(const KUrl&)) );

	connect(d->mRecentFoldersView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotListViewClicked(const QModelIndex&)) );
}


StartPage::~StartPage() {
	delete d;
}


void StartPage::applyPalette(const QPalette& newPalette) {
	QColor fgColor = newPalette.text().color();

	QPalette pal = palette();
	pal.setBrush(backgroundRole(), newPalette.base());
	setPalette(pal);

	QString css = QString::fromUtf8(
		"QLabel, QListView {"
		"	color: %1;"
		"}"

		"QLabel {"
		"	font-weight: bold;"
		"	border-bottom: 1px solid %1;"
		"}"

		"QListView {"
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
	emit urlSelected(url);
}


void StartPage::showEvent(QShowEvent* event) {
	d->updateRecentFoldersModel();
	QFrame::showEvent(event);
}


} // namespace
