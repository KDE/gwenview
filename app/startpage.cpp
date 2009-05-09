// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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

#include <config-gwenview.h>

// Qt
#include <QListView>
#include <QMenu>
#include <QStyledItemDelegate>

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

/**
 * Inherit from QStyledItemDelegate to match KFilePlacesViewDelegate sizeHint
 * height.
 */
class HistoryViewDelegate : public QStyledItemDelegate {
public:
	HistoryViewDelegate(QObject* parent = 0)
	: QStyledItemDelegate(parent) {}

	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
		QSize sh = QStyledItemDelegate::sizeHint(option, index);
		int iconSize = static_cast<QAbstractItemView*>(parent())->iconSize().height();
		// Copied from KFilePlacesViewDelegate::sizeHint()
		int height = option.fontMetrics.height() / 2 + qMax(iconSize, option.fontMetrics.height());
		sh.setHeight(qMax(sh.height(), height));
		return sh;
	}
};


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

	void updateHistoryTab() {
		mHistoryWidget->setVisible(GwenviewConfig::historyEnabled());
		mHistoryDisabledLabel->setVisible(!GwenviewConfig::historyEnabled());
	}
};

static void initViewPalette(QAbstractItemView* view, const QColor& fgColor) {
	QWidget* viewport = view->viewport();
	QPalette palette = viewport->palette();
	palette.setColor(viewport->backgroundRole(), Qt::transparent);
	palette.setColor(QPalette::WindowText, fgColor);
	palette.setColor(QPalette::Text, fgColor);

	// QListView uses QStyledItemDelegate, which uses the view palette for
	// foreground color, while KFilePlacesView uses the viewport palette.
	viewport->setPalette(palette);
	view->setPalette(palette);
}

StartPage::StartPage(QWidget* parent, GvCore* gvCore)
: QFrame(parent)
, d(new StartPagePrivate) {
	d->that = this;
	d->mGvCore = gvCore;
	d->setupUi(this);
	setFrameStyle(QFrame::NoFrame);

	// Bookmark view
	d->mBookmarksModel = new KFilePlacesModel(this);

	d->mBookmarksView->setModel(d->mBookmarksModel);
	d->mBookmarksView->setAutoResizeItemsEnabled(false);

	connect(d->mBookmarksView, SIGNAL(urlChanged(const KUrl&)),
		SIGNAL(urlSelected(const KUrl&)) );

	// Tag view
	connect(d->mTagView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotTagViewClicked(const QModelIndex&)));

	// Recent folder view
	d->mRecentFoldersView->setItemDelegate(new HistoryViewDelegate(d->mRecentFoldersView));
	connect(d->mRecentFoldersView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotListViewClicked(const QModelIndex&)) );

	connect(d->mRecentFoldersView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showRecentFoldersViewContextMenu(const QPoint&)));

	// Url bag view
	d->mRecentUrlsView->setItemDelegate(new HistoryViewDelegate(d->mRecentUrlsView));
	connect(d->mRecentUrlsView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotListViewClicked(const QModelIndex&)) );

	connect(d->mRecentUrlsView, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showRecentFoldersViewContextMenu(const QPoint&)));

	d->setupSearchUi(gvCore->semanticInfoBackEnd());

	d->updateHistoryTab();
	connect(GwenviewConfig::self(), SIGNAL(configChanged()),
		SLOT(slotConfigChanged()));
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
	pal.setBrush(QPalette::Button, newPalette.base());
	pal.setBrush(QPalette::WindowText, fgColor);
	pal.setBrush(QPalette::ButtonText, fgColor);
	pal.setBrush(QPalette::Text, fgColor);
	setPalette(pal);

	initViewPalette(d->mBookmarksView, fgColor);
	initViewPalette(d->mTagView, fgColor);
	initViewPalette(d->mRecentFoldersView, fgColor);
	initViewPalette(d->mRecentUrlsView, fgColor);

	QString css = QString::fromUtf8(
		"font-weight: bold;"
		"color: %1;"
		"border: 1px solid transparent;"
		"border-bottom-color: %1;"
		)
		.arg(fgColor.name());
	Q_FOREACH(QLabel* label, findChildren<QLabel*>()) {
		// Set css by hand on each label because when a css is applied to the
		// whole widget, it does not use native tabs anymore.
		if (label->property("title").isValid()) {
			label->setStyleSheet(css);
		}
	}
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
	if (GwenviewConfig::historyEnabled()) {
		if (!d->mRecentFoldersView->model()) {
			d->mRecentFoldersView->setModel(d->mGvCore->recentFoldersModel());
		}
		if (!d->mRecentUrlsView->model()) {
			d->mRecentUrlsView->setModel(d->mGvCore->recentUrlsModel());
		}
	}
	QFrame::showEvent(event);
}


void StartPage::showRecentFoldersViewContextMenu(const QPoint& pos) {
	QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
	KUrl url;
	QModelIndex index = view->indexAt(pos);
	if (index.isValid()) {
		QVariant data = index.data(KFilePlacesModel::UrlRole);
		url = data.toUrl();
	}

	// Create menu
	QMenu menu(this);
	bool fromRecentUrls = view == d->mRecentUrlsView;
	QAction* addToPlacesAction = fromRecentUrls ? 0 : menu.addAction(KIcon("bookmark-new"), i18n("Add to Places"));
	QAction* removeAction = menu.addAction(KIcon("edit-delete"), fromRecentUrls ? i18n("Forget this URL") : i18n("Forget this Folder"));
	menu.addSeparator();
	QAction* clearAction = menu.addAction(KIcon("edit-delete-all"), i18n("Forget All"));

	if (!index.isValid()) {
		if (addToPlacesAction) {
			addToPlacesAction->setEnabled(false);
		}
		removeAction->setEnabled(false);
	}

	// Handle menu
	QAction* action = menu.exec(view->mapToGlobal(pos));
	if (!action) {
		return;
	}
	if (action == addToPlacesAction) {
		QString text = url.fileName();
		if (text.isEmpty()) {
			text = url.pathOrUrl();
		}
		d->mBookmarksModel->addPlace(text, url);
	} else if (action == removeAction) {
		view->model()->removeRow(index.row());
	} else if (action == clearAction) {
		view->model()->removeRows(0, view->model()->rowCount());
	}
}


void StartPage::slotConfigChanged() {
	d->updateHistoryTab();
}


} // namespace
