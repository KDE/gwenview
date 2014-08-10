// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include "startmainpage.h"

#include <config-gwenview.h>

// Qt
#include <QListView>
#include <QMenu>
#include <QPlastiqueStyle>
#include <QStyledItemDelegate>

// KDE
#include <KFilePlacesModel>
#include <KGlobalSettings>
#include <QIcon>
#include <KMimeType>

// Local
#include <gvcore.h>
#include <ui_startmainpage.h>
#include <lib/flowlayout.h>
#include <lib/gwenviewconfig.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <lib/thumbnailview/previewitemdelegate.h>

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <lib/semanticinfo/tagmodel.h>
#endif

namespace Gwenview
{

class HistoryThumbnailViewHelper : public AbstractThumbnailViewHelper
{
public:
    HistoryThumbnailViewHelper(QObject* parent)
        : AbstractThumbnailViewHelper(parent)
    {}

    virtual void showContextMenu(QWidget*)
    {
    }

    virtual void showMenuForUrlDroppedOnViewport(QWidget*, const KUrl::List&)
    {
    }

    virtual void showMenuForUrlDroppedOnDir(QWidget*, const KUrl::List&, const KUrl&)
    {
    }
};

/**
 * Inherit from QStyledItemDelegate to match KFilePlacesViewDelegate sizeHint
 * height.
 */
class HistoryViewDelegate : public QStyledItemDelegate
{
public:
    HistoryViewDelegate(QObject* parent = 0)
        : QStyledItemDelegate(parent)
        {}

    virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSize sh = QStyledItemDelegate::sizeHint(option, index);
        int iconSize = static_cast<QAbstractItemView*>(parent())->iconSize().height();
        // Copied from KFilePlacesViewDelegate::sizeHint()
        int height = option.fontMetrics.height() / 2 + qMax(iconSize, option.fontMetrics.height());
        sh.setHeight(qMax(sh.height(), height));
        return sh;
    }
};

struct StartMainPagePrivate : public Ui_StartMainPage
{
    StartMainPage* q;
    GvCore* mGvCore;
    KFilePlacesModel* mBookmarksModel;
    bool mSearchUiInitialized;

    void setupSearchUi()
    {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_BALOO
        mTagView->setModel(TagModel::createAllTagsModel(mTagView, mGvCore->semanticInfoBackEnd()));
        mTagView->show();
        mTagLabel->hide();
#else
        mTagView->hide();
        mTagLabel->hide();
#endif
    }

    void updateHistoryTab()
    {
        mHistoryWidget->setVisible(GwenviewConfig::historyEnabled());
        mHistoryDisabledLabel->setVisible(!GwenviewConfig::historyEnabled());
    }
};

static void initViewPalette(QAbstractItemView* view, const QColor& fgColor)
{
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

static bool styleIsGtkBased()
{
    const char* name = QApplication::style()->metaObject()->className();
    return qstrcmp(name, "QGtkStyle") == 0;
}

StartMainPage::StartMainPage(QWidget* parent, GvCore* gvCore)
: QFrame(parent)
, d(new StartMainPagePrivate)
{
    d->q = this;
    d->mGvCore = gvCore;
    d->mSearchUiInitialized = false;

    d->setupUi(this);
    if (styleIsGtkBased()) {
        // Gtk-based styles do not apply the correct background color on tabs.
        // As a workaround, use the Plastique style instead.
        QStyle* fix = new QPlastiqueStyle();
        fix->setParent(this);
        d->mHistoryWidget->tabBar()->setStyle(fix);
        d->mPlacesTagsWidget->tabBar()->setStyle(fix);
    }
    setFrameStyle(QFrame::NoFrame);

    // Bookmark view
    d->mBookmarksModel = new KFilePlacesModel(this);

    d->mBookmarksView->setModel(d->mBookmarksModel);
    d->mBookmarksView->setAutoResizeItemsEnabled(false);

    connect(d->mBookmarksView, SIGNAL(urlChanged(KUrl)),
            SIGNAL(urlSelected(KUrl)));

    // Tag view
    connect(d->mTagView, SIGNAL(clicked(QModelIndex)),
            SLOT(slotTagViewClicked(QModelIndex)));

    // Recent folder view
    connect(d->mRecentFoldersView, SIGNAL(indexActivated(QModelIndex)),
            SLOT(slotListViewActivated(QModelIndex)));

    connect(d->mRecentFoldersView, SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(showRecentFoldersViewContextMenu(QPoint)));

    // Url bag view
    d->mRecentUrlsView->setItemDelegate(new HistoryViewDelegate(d->mRecentUrlsView));

    connect(d->mRecentUrlsView, SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(showRecentFoldersViewContextMenu(QPoint)));

    if (KGlobalSettings::singleClick()) {
        if (KGlobalSettings::changeCursorOverIcon()) {
            d->mRecentUrlsView->setCursor(Qt::PointingHandCursor);
        }
        connect(d->mRecentUrlsView, SIGNAL(clicked(QModelIndex)),
                SLOT(slotListViewActivated(QModelIndex)));
    } else {
        connect(d->mRecentUrlsView, SIGNAL(doubleClicked(QModelIndex)),
                SLOT(slotListViewActivated(QModelIndex)));
    }

    d->updateHistoryTab();
    connect(GwenviewConfig::self(), SIGNAL(configChanged()),
            SLOT(loadConfig()));

    d->mRecentFoldersView->setFocus();
}

StartMainPage::~StartMainPage()
{
    delete d;
}

void StartMainPage::slotTagViewClicked(const QModelIndex& index)
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_BALOO
    if (!index.isValid()) {
        return;
    }
    // FIXME: Check label encoding
    const QString tag = index.data().toString();
    emit urlSelected(KUrl("tags:/" + tag));
#endif
}

void StartMainPage::applyPalette(const QPalette& newPalette)
{
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
}

void StartMainPage::slotListViewActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }
    QVariant data = index.data(KFilePlacesModel::UrlRole);
    KUrl url = data.toUrl();

    // Prevent dir lister error
    if (!url.isValid()) {
        qCritical() << "Tried to open an invalid url";
        return;
    }
    emit urlSelected(url);
}

void StartMainPage::showEvent(QShowEvent* event)
{
    if (GwenviewConfig::historyEnabled()) {
        if (!d->mRecentFoldersView->model()) {
            d->mRecentFoldersView->setThumbnailViewHelper(new HistoryThumbnailViewHelper(d->mRecentFoldersView));
            d->mRecentFoldersView->setModel(d->mGvCore->recentFoldersModel());
            PreviewItemDelegate* delegate = new PreviewItemDelegate(d->mRecentFoldersView);
            delegate->setContextBarActions(PreviewItemDelegate::NoAction);
            delegate->setTextElideMode(Qt::ElideLeft);
            d->mRecentFoldersView->setItemDelegate(delegate);
            d->mRecentFoldersView->setThumbnailWidth(128);
            d->mRecentFoldersView->setCreateThumbnailsForRemoteUrls(false);
            QModelIndex index = d->mRecentFoldersView->model()->index(0, 0);
            if (index.isValid()) {
                d->mRecentFoldersView->setCurrentIndex(index);
            }
        }
        if (!d->mRecentUrlsView->model()) {
            d->mRecentUrlsView->setModel(d->mGvCore->recentUrlsModel());
        }
    }
    if (!d->mSearchUiInitialized) {
        d->mSearchUiInitialized = true;
        d->setupSearchUi();
    }
    QFrame::showEvent(event);
}

void StartMainPage::showRecentFoldersViewContextMenu(const QPoint& pos)
{
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
    QAction* addToPlacesAction = fromRecentUrls ? 0 : menu.addAction(QIcon::fromTheme("bookmark-new"), i18n("Add to Places"));
    QAction* removeAction = menu.addAction(QIcon::fromTheme("edit-delete"), fromRecentUrls ? i18n("Forget this URL") : i18n("Forget this Folder"));
    menu.addSeparator();
    QAction* clearAction = menu.addAction(QIcon::fromTheme("edit-delete-all"), i18n("Forget All"));

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

void StartMainPage::loadConfig()
{
    d->updateHistoryTab();
    applyPalette(d->mGvCore->palette(GvCore::NormalViewPalette));
}

ThumbnailView* StartMainPage::recentFoldersView() const
{
    return d->mRecentFoldersView;
}

} // namespace
