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
#ifndef FILEOPSCONTEXTMANAGERITEM_H
#define FILEOPSCONTEXTMANAGERITEM_H

// Qt

// KDE
#include <KNewFileMenu>
#include <KService>

// Local
#include "abstractcontextmanageritem.h"

class QAction;
class QMimeData;
class QListView;
class KActionCollection;
class KXMLGUIClient;

namespace Gwenview
{
class SideBarGroup;

class FileOpsContextManagerItem : public AbstractContextManagerItem
{
    Q_OBJECT
public:
    FileOpsContextManagerItem(Gwenview::ContextManager* manager, QListView* thumbnailView, KActionCollection* actionCollection, KXMLGUIClient* client);
    ~FileOpsContextManagerItem() override;

private Q_SLOTS:
    void updateActions();
    void updatePasteAction();
    void updateSideBarContent();

    void cut();
    void copy();
    void paste();
    void rename();
    void copyTo();
    void moveTo();
    void linkTo();
    void trash();
    void del();
    void restore();
    void showProperties();
    void createFolder();
    void populateOpenMenu();
    void openWith(QAction* action);
    void openContainingFolder();

private:
    QList<QUrl> urlList() const;
    void updateServiceList();
    QMimeData* selectionMimeData();
    QUrl pasteTargetUrl() const;

    QListView* mThumbnailView;
    KXMLGUIClient* mXMLGUIClient;
    SideBarGroup* mGroup;
    QAction * mCutAction;
    QAction * mCopyAction;
    QAction * mPasteAction;
    QAction * mCopyToAction;
    QAction * mMoveToAction;
    QAction * mLinkToAction;
    QAction * mRenameAction;
    QAction * mTrashAction;
    QAction * mDelAction;
    QAction * mRestoreAction;
    QAction * mShowPropertiesAction;
    QAction * mCreateFolderAction;
    QAction * mOpenWithAction;
    QAction * mOpenContainingFolderAction;
    QList<QAction*> mRegularFileActionList;
    QList<QAction*> mTrashFileActionList;
    KService::List mServiceList;
    KNewFileMenu * mNewFileMenu;
    bool mInTrash;
};

} // namespace

#endif /* FILEOPSCONTEXTMANAGERITEM_H */
