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

#ifdef GTK_WORKAROUND_BROKE_IN_KF5_PORT
#include <QPlastiqueStyle>
#endif

#include <QIcon>
#include <QStyledItemDelegate>

// KF
#include <KFilePlacesModel>
#include <KLocalizedString>

// Local
#include "gwenview_app_debug.h"
#include <gvcore.h>
#include <ui_startmainpage.h>
#include <lib/dialogguard.h>
#include <lib/flowlayout.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/scrollerutils.h>

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

    void showContextMenu(QWidget*) override
    {
    }

    void showMenuForUrlDroppedOnViewport(QWidget*, const QList<QUrl>&) override
    {
    }

    void showMenuForUrlDroppedOnDir(QWidget*, const QList<QUrl>&, const QUrl&) override
    {
    }
};

struct StartMainPagePrivate : public Ui_StartMainPage
{
    StartMainPage* q;
    GvCore* mGvCore;
    KFilePlacesModel* mBookmarksModel;
    ThumbnailProvider *mRecentFilesThumbnailProvider;
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

    void setupHistoryView(ThumbnailView *view)
    {
        view->setThumbnailViewHelper(new HistoryThumbnailViewHelper(view));
        PreviewItemDelegate* delegate = new PreviewItemDelegate(view);
        delegate->setContextBarActions(PreviewItemDelegate::NoAction);
        delegate->setTextElideMode(Qt::ElideLeft);
        view->setItemDelegate(delegate);
        view->setThumbnailWidth(128);
        view->setCreateThumbnailsForRemoteUrls(false);
        QModelIndex index = view->model()->index(0, 0);
        if (index.isValid()) {
            view->setCurrentIndex(index);
        }
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
    d->mRecentFilesThumbnailProvider = nullptr;
    d->q = this;
    d->mGvCore = gvCore;
    d->mSearchUiInitialized = false;

    d->setupUi(this);
    if (styleIsGtkBased()) {
        #ifdef GTK_WORKAROUND_BROKE_IN_KF5_PORT

        // Gtk-based styles do not apply the correct background color on tabs.
        // As a workaround, use the Plastique style instead.
        QStyle* fix = new QPlastiqueStyle();
        fix->setParent(this);
        d->mHistoryWidget->tabBar()->setStyle(fix);
        d->mPlacesTagsWidget->tabBar()->setStyle(fix);
        #endif
    }
    setFrameStyle(QFrame::NoFrame);

    // Bookmark view
    d->mBookmarksModel = new KFilePlacesModel(this);

    d->mBookmarksView->setModel(d->mBookmarksModel);
    d->mBookmarksView->setAutoResizeItemsEnabled(false);

    connect(d->mBookmarksView, &KFilePlacesView::urlChanged, this, &StartMainPage::urlSelected);

    // Tag view
    connect(d->mTagView, &QListView::clicked, this, &StartMainPage::slotTagViewClicked);

    // Recent folders view
    connect(d->mRecentFoldersView, &Gwenview::ThumbnailView::indexActivated,
            this, &StartMainPage::slotListViewActivated);
    connect(d->mRecentFoldersView, &Gwenview::ThumbnailView::customContextMenuRequested,
            this, &StartMainPage::showContextMenu);

    // Recent files view
    connect(d->mRecentFilesView, &Gwenview::ThumbnailView::indexActivated,
            this, &StartMainPage::slotListViewActivated);
    connect(d->mRecentFilesView, &Gwenview::ThumbnailView::customContextMenuRequested,
            this, &StartMainPage::showContextMenu);

    d->updateHistoryTab();
    connect(GwenviewConfig::self(), &GwenviewConfig::configChanged, this, &StartMainPage::loadConfig);

    d->mRecentFoldersView->setFocus();

    ScrollerUtils::setQScroller(d->mBookmarksView->viewport());
    d->mBookmarksView->viewport()->installEventFilter(this);
}

StartMainPage::~StartMainPage()
{
    delete d->mRecentFilesThumbnailProvider;
    delete d;
}

bool StartMainPage::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->source() == Qt::MouseEventSynthesizedByQt) {
            return true;
        }
    }
    return false;
}

void StartMainPage::slotTagViewClicked(const QModelIndex& index)
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_BALOO
    if (!index.isValid()) {
        return;
    }
    // FIXME: Check label encoding
    const QString tag = index.data().toString();
    emit urlSelected(QUrl("tags:/" + tag));
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
    initViewPalette(d->mRecentFilesView, fgColor);
}

void StartMainPage::slotListViewActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }
    QVariant data = index.data(KFilePlacesModel::UrlRole);
    QUrl url = data.toUrl();

    // Prevent dir lister error
    if (!url.isValid()) {
        qCCritical(GWENVIEW_APP_LOG) << "Tried to open an invalid url";
        return;
    }
    emit urlSelected(url);
}

void StartMainPage::showEvent(QShowEvent* event)
{
    if (GwenviewConfig::historyEnabled()) {
        if (!d->mRecentFoldersView->model()) {
            d->mRecentFoldersView->setModel(d->mGvCore->recentFoldersModel());
            d->setupHistoryView(d->mRecentFoldersView);
        }
        if (!d->mRecentFilesView->model()) {
            d->mRecentFilesView->setModel(d->mGvCore->recentFilesModel());
            d->mRecentFilesThumbnailProvider = new ThumbnailProvider();
            d->mRecentFilesView->setThumbnailProvider(d->mRecentFilesThumbnailProvider);
            d->setupHistoryView(d->mRecentFilesView);
        }
    }
    if (!d->mSearchUiInitialized) {
        d->mSearchUiInitialized = true;
        d->setupSearchUi();
    }
    QFrame::showEvent(event);
}

void StartMainPage::showContextMenu(const QPoint& pos)
{
    // Create menu
    DialogGuard<QMenu> menu(this);

    QAction* addAction = menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), QString());
    QAction* forgetAction = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), QString());
    menu->addSeparator();
    QAction* forgetAllAction = menu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete-all")), QString());

    if (d->mHistoryWidget->currentWidget() == d->mRecentFoldersTab) {
        addAction->setText(i18nc("@action Recent Folders view", "Add Folder to Places"));
        forgetAction->setText(i18nc("@action Recent Folders view", "Forget This Folder"));
        forgetAllAction->setText(i18nc("@action Recent Folders view", "Forget All Folders"));
    } else if (d->mHistoryWidget->currentWidget() == d->mRecentFilesTab) {
        addAction->setText(i18nc("@action Recent Files view", "Add Containing Folder to Places"));
        forgetAction->setText(i18nc("@action Recent Files view", "Forget This File"));
        forgetAllAction->setText(i18nc("@action Recent Files view", "Forget All Files"));
    } else {
        GV_WARN_AND_RETURN("Context menu not implemented on this tab page");
    }

    const QAbstractItemView* view = qobject_cast<QAbstractItemView*>(sender());
    const QModelIndex index = view->indexAt(pos);
    addAction->setEnabled(index.isValid());
    forgetAction->setEnabled(index.isValid());

    // Handle menu
    const QAction* action = menu->exec(view->mapToGlobal(pos));
    if (!action) {
        return;
    }

    const QVariant data = index.data(KFilePlacesModel::UrlRole);
    QUrl url = data.toUrl();
    if (action == addAction) {
        if (d->mHistoryWidget->currentWidget() == d->mRecentFilesTab) {
            url = url.adjusted(QUrl::RemoveFilename);
        }
        QString text = url.adjusted(QUrl::StripTrailingSlash).fileName();
        if (text.isEmpty()) {
            text = url.toDisplayString();
        }
        d->mBookmarksModel->addPlace(text, url);
    } else if (action == forgetAction) {
        view->model()->removeRow(index.row());
        if (d->mHistoryWidget->currentWidget() == d->mRecentFilesTab) {
            emit recentFileRemoved(url);
        }
    } else if (action == forgetAllAction) {
        view->model()->removeRows(0, view->model()->rowCount());
        if (d->mHistoryWidget->currentWidget() == d->mRecentFilesTab) {
            emit recentFilesCleared();
        }
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
