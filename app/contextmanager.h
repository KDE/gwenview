/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef CONTEXTMANAGER_H
#define CONTEXTMANAGER_H

// Qt
#include <QList>
#include <QObject>

// KDE
#include <kurl.h>
#include <kfileitem.h>

namespace Gwenview {

class SideBar;

class ImageViewPart;
class AbstractContextManagerItem;

/**
 * Manage the update of the contextual parts of the applications, 
 * like the sidebar or the context menu.
 */
class ContextManager : public QObject {
	Q_OBJECT
public:
	ContextManager(QObject* parent);

	~ContextManager();

	void addItem(AbstractContextManagerItem* item);

	void setCurrentUrl(const KUrl&);

	KUrl currentUrl() const;

	void setCurrentDirUrl(const KUrl&);

	KUrl currentDirUrl() const;

	void setSelection(const KFileItemList& itemList);

	KFileItemList selection() const;

	void setSideBar(SideBar*);

Q_SIGNALS:
	void selectionChanged();
	void currentDirUrlChanged();

private:
	//FIXME: use d pointer
	QList<AbstractContextManagerItem*> mList;
	KFileItemList mSelection;
	SideBar* mSideBar;
	KUrl mCurrentDirUrl;
	KUrl mCurrentUrl;
};

} // namespace

#endif /* CONTEXTMANAGER_H */
