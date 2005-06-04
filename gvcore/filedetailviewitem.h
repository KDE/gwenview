// vim: set tabstop=4 shiftwidth=4 noexpandtab
/* This file is based on kfiledetailview.h from the KDE libs. Original
   copyright follows.
*/
/* This file is part of the KDE libraries
	Copyright (C) 1997 Stephan Kulow <coolo@kde.org>

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public License
	along with this library; see the file COPYING.LIB.	If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef FILEDETAILVIEWITEM_H
#define FILEDETAILVIEWITEM_H

// KDE includes
#include <klistview.h>
#include <kfileitem.h>
namespace Gwenview {

#define COL_NAME 0
#define COL_SIZE 1
#define COL_DATE 2
#define COL_PERM 3
#define COL_OWNER 4
#define COL_GROUP 5

class FileDetailViewItem : public KListViewItem
{
public:
	FileDetailViewItem( QListView* parent, const QString &text,
					   const QPixmap &icon, KFileItem* fi )
		: KListViewItem( parent, text ), inf( fi ) {
		setPixmap( 0, icon );
		setText( 0, text );
	}

	FileDetailViewItem( QListView* parent, KFileItem* fi )
		: KListViewItem( parent ), inf( fi ) {
		init();
	}

	FileDetailViewItem( QListView* parent, const QString &text,
					   const QPixmap &icon, KFileItem* fi,
					   QListViewItem* after)
		: KListViewItem( parent, after ), inf( fi ) {
		setPixmap( 0, icon );
		setText( 0, text );
	}
	
	~FileDetailViewItem() {
		inf->removeExtraData( listView() );
	}

	KFileItem* fileInfo() const { return inf; }

	virtual QString key( int /*column*/, bool /*ascending*/ ) const { return m_key; }

	void setKey( const QString& key ) { m_key = key; }

	QRect rect() const
	{
		QRect r = listView()->itemRect(this);
		return QRect( listView()->viewportToContents( r.topLeft() ),
					  QSize( r.width(), r.height() ) );
	}

	void init();
	void paintCell(QPainter*,const QColorGroup &,int column,int width,int align);
	
private:
	KFileItem* inf;
	QString m_key;
};

} // namespace
#endif

