/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "contextmanager.moc"

// KDE
#include <kfileitem.h>

// Local
#include "sidebar.h"
#include "abstractcontextmanageritem.h"

namespace Gwenview {

ContextManager::ContextManager(QObject* parent)
: QObject(parent) {}


ContextManager::~ContextManager() {
	Q_FOREACH(AbstractContextManagerItem* item, mList) {
		delete item;
	}
}


void ContextManager::setSideBar(SideBar* sideBar) {
	mSideBar = sideBar;
}


void ContextManager::addItem(AbstractContextManagerItem* item) {
	Q_ASSERT(mSideBar);
	mList << item;
	item->setSideBar(mSideBar);
}


void ContextManager::setSelection(const QList<KFileItem>& list) {
	mSelection = list;
	selectionChanged();
}


QList<KFileItem> ContextManager::selection() const {
	return mSelection;
}


void ContextManager::setImageView(ImageViewPart* imageView) {
	Q_FOREACH(AbstractContextManagerItem* item, mList) {
		item->setImageView(imageView);
	}
}


} // namespace
