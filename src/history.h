// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
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
#ifndef GVHISTORY_H
#define GVHISTORY_H

// Qt
#include <qobject.h>
#include <qvaluelist.h>

// KDE
#include <kurl.h>

class KToolBarPopupAction;
class KActionCollection;

typedef QValueList<KURL> HistoryList;

class GVHistory : public QObject {
Q_OBJECT
	
public:
	GVHistory(KActionCollection*);
	~GVHistory();

signals:
	void urlChanged(const KURL&);

public slots:
	void addURLToHistory(const KURL&);

private:
	KToolBarPopupAction* mGoBack;
	KToolBarPopupAction* mGoForward;
	HistoryList mHistoryList;
	HistoryList::Iterator mPosition;
	bool mMovingInHistory;
	
private slots:
	void fillGoBackMenu();
	void fillGoForwardMenu();
	void goBack();
	void goForward();
	void goBackTo(int);
	void goForwardTo(int);
};

#endif


