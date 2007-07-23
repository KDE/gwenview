// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "imageopscontextmanageritem.moc"

// Qt
#include <QAction>

// KDE
#include <klocale.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

struct ImageOpsContextManagerItemPrivate {
	QList<QAction*> mActionList;
	SideBar* mSideBar;
	SideBarGroup* mGroup;
};


ImageOpsContextManagerItem::ImageOpsContextManagerItem(ContextManager* manager, QList<QAction*> actionList)
: AbstractContextManagerItem(manager)
, d(new ImageOpsContextManagerItemPrivate) {
	d->mActionList = actionList;
	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );
}


ImageOpsContextManagerItem::~ImageOpsContextManagerItem() {
	delete d;
}


void ImageOpsContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );

	d->mGroup = sideBar->createGroup(i18n("Image Operations"));
}


void ImageOpsContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar->isVisible()) {
		return;
	}

	d->mGroup->clear();
	bool notEmpty = false;
	Q_FOREACH(QAction* action, d->mActionList) {
		if (action->isEnabled()) {
			d->mGroup->addAction(action);
			notEmpty = true;
		}
	}
	if (notEmpty) {
		d->mGroup->show();
	} else {
		d->mGroup->hide();
	}
}


} // namespace
