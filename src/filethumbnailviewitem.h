/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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

#ifndef FILETHUMBNAILVIEWITEM_H
#define FILETHUMBNAILVIEWITEM_H

// Qt includes
#include <qiconview.h>
#include <qstring.h>


class QFontMetrics;
class QPixmap;
class KFileItem;
class KWordWrap;


/**
 * We override the QIconViewItem to control the look of selected items
 * and get a pointer to our KFileItem
 */
class FileThumbnailViewItem : public QIconViewItem {
public:
	FileThumbnailViewItem(QIconView* parent,const QString& text,const QPixmap& icon, KFileItem* fileItem);
	~FileThumbnailViewItem();

	KFileItem* fileItem() const { return mFileItem; }

protected:
	void paintItem(QPainter* painter, const QColorGroup& colorGroup);
	void calcRect( const QString& text_=QString::null );
	void truncateText(const QFontMetrics&);
	void paintFocus(QPainter*, const QColorGroup&) {}
	
	KFileItem* mFileItem;
	KWordWrap* mWordWrap;
	QString mTruncatedText;
};

#endif
