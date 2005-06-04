// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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
#ifndef BOOKMARKOWNER_H
#define BOOKMARKOWNER_H


// KDE includes
#include <kbookmarkmanager.h>
#include <kurl.h>
namespace Gwenview {


class BookmarkOwner : public QObject, public KBookmarkOwner {
Q_OBJECT
public:
	BookmarkOwner(QWidget* parent);

	// KBookmarkOwner interface
	void openBookmarkURL(const QString&);
	QString currentURL() const;
	
public slots:
	void setURL(const KURL&);

signals:
	void openURL(const KURL&);

private:
	KURL mURL;
};


} // namespace
#endif

