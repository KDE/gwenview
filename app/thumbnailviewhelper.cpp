// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "thumbnailviewhelper.h"

#include <config-gwenview.h>

// Qt
#include <QAction>
#include <QCursor>
#include <QMenu>

// KF
#include <KActionCollection>

// Local
#include "fileoperations.h"
#include "gwenview_app_debug.h"
#include <lib/document/documentfactory.h>

namespace Gwenview
{
struct ThumbnailViewHelperPrivate {
    KActionCollection *mActionCollection = nullptr;
    QUrl mCurrentDirUrl;

    void addActionToMenu(QMenu &popup, const QString &name)
    {
        QAction *action = mActionCollection->action(name);
        if (!action) {
            qCWarning(GWENVIEW_APP_LOG) << "Unknown action" << name;
            return;
        }
        if (action->isEnabled()) {
            popup.addAction(action);
        }
    }
};

ThumbnailViewHelper::ThumbnailViewHelper(QObject *parent, KActionCollection *actionCollection)
    : AbstractThumbnailViewHelper(parent)
    , d(new ThumbnailViewHelperPrivate)
{
    d->mActionCollection = actionCollection;
}

ThumbnailViewHelper::~ThumbnailViewHelper()
{
    delete d;
}

void ThumbnailViewHelper::setCurrentDirUrl(const QUrl &url)
{
    d->mCurrentDirUrl = url;
}

void ThumbnailViewHelper::showContextMenu(QWidget *parent)
{
    QMenu popup(parent);
    if (d->mCurrentDirUrl.scheme() == QLatin1String("trash")) {
        d->addActionToMenu(popup, QStringLiteral("file_restore"));
        d->addActionToMenu(popup, QStringLiteral("deletefile"));
        popup.addSeparator();
        d->addActionToMenu(popup, QStringLiteral("file_show_properties"));
    } else {
        d->addActionToMenu(popup, QStringLiteral("file_create_folder"));
        popup.addSeparator();
        d->addActionToMenu(popup, QStringLiteral("file_rename"));
        d->addActionToMenu(popup, QStringLiteral("file_trash"));
        d->addActionToMenu(popup, QStringLiteral("deletefile"));
        popup.addSeparator();
        d->addActionToMenu(popup, KStandardAction::name(KStandardAction::Copy));
        d->addActionToMenu(popup, QStringLiteral("file_copy_to"));
        d->addActionToMenu(popup, QStringLiteral("file_move_to"));
        d->addActionToMenu(popup, QStringLiteral("file_link_to"));
        popup.addSeparator();
        d->addActionToMenu(popup, QStringLiteral("file_open_in_new_window"));
        d->addActionToMenu(popup, QStringLiteral("file_open_with"));
        d->addActionToMenu(popup, QStringLiteral("file_open_containing_folder"));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        d->addActionToMenu(popup, QStringLiteral("edit_tags"));
#endif
        popup.addSeparator();
        d->addActionToMenu(popup, QStringLiteral("file_show_properties"));
    }
    popup.exec(QCursor::pos());
}

void ThumbnailViewHelper::showMenuForUrlDroppedOnViewport(QWidget *parent, const QList<QUrl> &lst)
{
    showMenuForUrlDroppedOnDir(parent, lst, d->mCurrentDirUrl);
}

void ThumbnailViewHelper::showMenuForUrlDroppedOnDir(QWidget *parent, const QList<QUrl> &urlList, const QUrl &destUrl)
{
    FileOperations::showMenuForDroppedUrls(parent, urlList, destUrl);
}

} // namespace

#include "moc_thumbnailviewhelper.cpp"
