/*  Gwenview - A simple image viewer for KDE
    Copyright (C) 2000-2002 Aurélien Gâteau
    This class is based on the KIconViewItem class from KDE libs.
    Original copyright follows.
*/
/* This file is part of the KDE libraries
   Copyright (C) 1999 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
// Qt includes
#include <qapp.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>

// KDE includes
#include <kdebug.h>
#include <kwordwrap.h>

// Our includes
#include "filethumbnailviewitem.h"


FileThumbnailViewItem::FileThumbnailViewItem(QIconView* view,const QString& text,const QPixmap& icon, KFileItem* fileItem)
: QIconViewItem(view,text,icon), mFileItem(fileItem), mWordWrap(0L) {
	calcRect();
}


FileThumbnailViewItem::~FileThumbnailViewItem() {
	delete mWordWrap;
}


void FileThumbnailViewItem::calcRect(const QString& text_) {
	QIconView *view = iconView();
	Q_ASSERT(view);
	if (!view) return;

	QFontMetrics fm = view->fontMetrics();

	delete mWordWrap;
	mWordWrap = 0L;

	QRect itemIconRect = pixmapRect();
	QRect itemTextRect = textRect();
	QRect itemRect = rect();

	int pw = 0;
	int ph = 0;

#ifndef QT_NO_PICTURE
	if ( picture() ) {
		QRect br = picture()->boundingRect();
		pw = br.width() + 2;
		ph = br.height() + 2;
	} else
#endif
	{
		// Qt uses unknown_icon if no pixmap. Let's see if we need that - I doubt it
		if (!pixmap()) return;
		pw = pixmap()->width() + 2;
		ph = pixmap()->height() + 2;
	}
	itemIconRect.setWidth( pw );
	itemIconRect.setHeight( ph );

	// When is text_ set ? Doesn't look like it's ever set.
	QString t = text_.isEmpty() ? text() : text_;

	int tw = 0;
	int th = 0;
	QRect outerRect( 0, 0, view->maxItemWidth() -
					( view->itemTextPos() == QIconView::Bottom ? 0 :
					pixmapRect().width() ), 0xFFFFFFFF );

// Calculate the word-wrap
	mWordWrap=KWordWrap::formatText( fm, outerRect, AlignHCenter | WordBreak /*| BreakAnywhere*/, t );
	QRect r;
	if (iconView()->wordWrapIconText()) {
		r=mWordWrap->boundingRect();
	} else {
		r=QRect(0,0,outerRect.width(),fm.height());
	}
	r.setWidth( r.width() + 4 );
	// [Non-word-wrap code removed]

	if ( r.width() > view->maxItemWidth() -
		( view->itemTextPos() == QIconView::Bottom ? 0 :
		pixmapRect().width() ) )
		r.setWidth( view->maxItemWidth() - ( view->itemTextPos() == QIconView::Bottom ? 0 :
												pixmapRect().width() ) );

	tw = r.width();
	th = r.height();
	int minw = fm.width( "X" );
	if ( tw < minw )
		tw = minw;

	itemTextRect.setWidth( tw );
	itemTextRect.setHeight( th );

// All this code isn't related to the word-wrap algo...
// Sucks that we have to duplicate it.
	int w = 0;    int h = 0;
	if ( view->itemTextPos() == QIconView::Bottom ) {
		w = QMAX( itemTextRect.width(), itemIconRect.width() );
		h = itemTextRect.height() + itemIconRect.height() + 1;

		itemRect.setWidth( w );
		itemRect.setHeight( h );
		int width = QMAX( w, QApplication::globalStrut().width() ); // see QIconViewItem::width()
		int height = QMAX( h, QApplication::globalStrut().height() ); // see QIconViewItem::height()
		itemTextRect = QRect( ( width - itemTextRect.width() ) / 2, height - itemTextRect.height(),
							itemTextRect.width(), itemTextRect.height() );
		itemIconRect = QRect( ( width - itemIconRect.width() ) / 2, 0,
							itemIconRect.width(), itemIconRect.height() );
	} else {
		h = QMAX( itemTextRect.height(), itemIconRect.height() );
		w = itemTextRect.width() + itemIconRect.width() + 1;

		itemRect.setWidth( w );
		itemRect.setHeight( h );
		int width = QMAX( w, QApplication::globalStrut().width() ); // see QIconViewItem::width()
		int height = QMAX( h, QApplication::globalStrut().height() ); // see QIconViewItem::height()

		itemTextRect = QRect( width - itemTextRect.width(), ( height - itemTextRect.height() ) / 2,
							itemTextRect.width(), itemTextRect.height() );
		if ( itemIconRect.height() > itemTextRect.height() ) // icon bigger than text -> center vertically
			itemIconRect = QRect( 0, ( height - itemIconRect.height() ) / 2,
								itemIconRect.width(), itemIconRect.height() );
		else // icon smaller than text -> center with first line
			itemIconRect = QRect( 0, ( fm.height() - itemIconRect.height() ) / 2,
								itemIconRect.width(), itemIconRect.height() );
	}

	if ( itemIconRect != pixmapRect() )
		setPixmapRect( itemIconRect );
	if ( itemTextRect != textRect() )
		setTextRect( itemTextRect );
	if ( itemRect != rect() )
		setItemRect( itemRect );

// Done by setPixmapRect, setTextRect and setItemRect !  [and useless if no rect changed]
//	view->updateItemContainer( this );
}



void FileThumbnailViewItem::paintItem(QPainter *p, const QColorGroup &cg) {
	QIconView* view = iconView();
	Q_ASSERT( view );
	if ( !view ) return;

	if ( !mWordWrap ) {
		kdWarning() << "KIconViewItem::paintItem called but wordwrap not ready - calcRect not called, or aborted!" << endl;
		return;
	}
	QRect pRect=pixmapRect(false);
	QRect tRect=textRect(false);

	p->save();

// Draw pixmap
	p->drawPixmap( pRect.x(), pRect.y(), *pixmap() );

// Draw focus
	if ( isSelected() ) {
		p->setPen( QPen( cg.highlight() ) );
		p->drawRect(pRect | tRect);
		p->fillRect( pRect.x(),tRect.y(),pRect.width(),tRect.height(), cg.highlight() );

		p->setPen( QPen( cg.highlightedText() ) );
	} else {
		if ( view->itemTextBackground() != NoBrush ) {
			p->fillRect( tRect, view->itemTextBackground() );
		}
		p->setPen( cg.text() );
	}

// Draw text
	int align = view->itemTextPos() == QIconView::Bottom ? AlignHCenter : AlignAuto;
	if (view->wordWrapIconText()) {
		mWordWrap->drawText( p, tRect.x(), tRect.y(), align );
	} else {
		p->drawText(tRect,align,mWordWrap->truncatedString());
	}

	p->restore();
}
