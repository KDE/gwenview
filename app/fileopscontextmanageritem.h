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

// Local
#include "abstractcontextmanageritem.h"

class QAction;
class QListView;
class KActionCollection;
class KXMLGUIClient;

namespace Gwenview
{

struct FileOpsContextManagerItemPrivate;
class FileOpsContextManagerItem : public AbstractContextManagerItem
{
    Q_OBJECT
public:
    FileOpsContextManagerItem(Gwenview::ContextManager* manager, QListView* thumbnailView, KActionCollection* actionCollection, KXMLGUIClient* client);
    ~FileOpsContextManagerItem();

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

private:
    FileOpsContextManagerItemPrivate* const d;
};

} // namespace

#endif /* FILEOPSCONTEXTMANAGERITEM_H */
