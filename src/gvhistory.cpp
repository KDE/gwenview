/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau
Copyright 2003 Tudor Calin

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// KDE 
#include <kaction.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kdeversion.h>

// Local
#include "gvpixmap.h"
#include "gvhistory.moc"

#if KDE_VERSION < 306
#define SAME_URL(u1,u2,slash) u1.cmp(u2,slash)
#else
#define SAME_URL(u1,u2,slash) u1.equals(u2,slash)
#endif

const unsigned int MAX_HISTORY_SIZE=12;

GVHistory::GVHistory(GVPixmap* gvPixmap, KActionCollection* actionCollection) {
	mGVPixmap=gvPixmap;
	mPosition=mHistoryList.end();
	mMovingInHistory=false;
	
	// Actions
	mGoBack=new KToolBarPopupAction(i18n("Back"), "back", ALT + Key_Left,
		this, SLOT(goBack()), actionCollection, "go_back");
	mGoForward=new KToolBarPopupAction(i18n("Forward"), "forward", ALT + Key_Right, 
		this, SLOT(goForward()), actionCollection, "go_forward");
	
	// Connections
	connect(mGoBack->popupMenu(),SIGNAL(activated(int)), 
		this,SLOT(goBackTo(int)) );
	connect(mGoForward->popupMenu(),SIGNAL(activated(int)), 
		this,SLOT(goForwardTo(int)) );

	connect(mGoBack->popupMenu(), SIGNAL(aboutToShow()),
		this, SLOT(fillGoBackMenu()) );
	connect(mGoForward->popupMenu(), SIGNAL(aboutToShow()),
		this, SLOT(fillGoForwardMenu()) );
	
	connect(mGVPixmap, SIGNAL(urlChanged(const KURL&,const QString&) ),
		this, SLOT(updateHistoryList(const KURL&)) );
}


GVHistory::~GVHistory() {
}


void GVHistory::updateHistoryList(const KURL& url) {
	if (!mMovingInHistory) {
		if (mPosition!=mHistoryList.end() && SAME_URL(url, *mPosition, true)) return;
		
		// Drop everything after current
		HistoryList::iterator it=mPosition;
		++it;
		mHistoryList.erase(it, mHistoryList.end());
		
		mHistoryList.append(url);
		if(mHistoryList.count()==MAX_HISTORY_SIZE) mHistoryList.pop_front();
		mPosition=mHistoryList.fromLast();
	}
		
	mGoBack->setEnabled(mPosition!=mHistoryList.begin());
	mGoForward->setEnabled(mPosition!=mHistoryList.fromLast());
}


void GVHistory::fillGoBackMenu() {
	QPopupMenu* menu=mGoBack->popupMenu();
	menu->clear();
	HistoryList::ConstIterator it;

	int pos=1;
	for(it=mHistoryList.begin(); it!=mPosition; ++it, ++pos) {
		menu->insertItem( (*it).prettyURL(-1), pos, 0);
	}
}

void GVHistory::fillGoForwardMenu() {
	QPopupMenu* menu=mGoForward->popupMenu();
	menu->clear();
	HistoryList::ConstIterator it=mPosition;
	++it;

	int pos=1;
	for(; it!=mHistoryList.end(); ++it, ++pos) {
		menu->insertItem( (*it).prettyURL(-1), pos, -1);
	}
}

void GVHistory::goBack() {
	goBackTo(1);
}


void GVHistory::goForward() {
	goForwardTo(1);
}


void GVHistory::goBackTo(int id) {
	for (;id>0; --id) --mPosition;
	mMovingInHistory=true;
	mGVPixmap->setDirURL(*mPosition);
	mMovingInHistory=false;
}


void GVHistory::goForwardTo(int id) {
	for (;id>0; --id) ++mPosition;
	mMovingInHistory=true;
	mGVPixmap->setDirURL(*mPosition);
	mMovingInHistory=false;
}
