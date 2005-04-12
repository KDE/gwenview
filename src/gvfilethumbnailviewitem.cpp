// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*  Gwenview - A simple image viewer for KDE
    Copyright 2000-2004 Aurélien Gâteau
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
#include <qapplication.h>
#include <qcolor.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>

// KDE includes
#include <kdebug.h>
#include <kwordwrap.h>
#include <kurldrag.h>

// Our includes
#include "gvfilethumbnailview.h"
#include "gvfilethumbnailviewitem.h"


#if 0
static void printRect(const QString& txt,const QRect& rect) {
	kdWarning() << txt << " : " << rect.x() << "x" << rect.y() << " " << rect.width() << "x" << rect.height() << endl;
}
#endif


static QString truncateText(const QFontMetrics& fm, const QString& fullText, int width) {
	static QString dots("...");
	QString text;

	// If the text fit in the width, don't truncate it
	if (fm.boundingRect(fullText).width()<=width) {
		return fullText;
	}

	// Find the number of letters to keep
	text=fullText;
	width-=fm.width(dots);
	int len=text.length();
	for(;len>0 && fm.width(text,len)>width;--len);

	// Truncate the text
	text.truncate(len);
	text+=dots;
	return text;
}


GVFileThumbnailViewItem::GVFileThumbnailViewItem(QIconView* view,const QString& text,const QPixmap& icon, KFileItem* fileItem)
: QIconViewItem(view,text,icon), mFileItem(fileItem), mWordWrap(0L) {
	updateInfoLines();
}


GVFileThumbnailViewItem::~GVFileThumbnailViewItem() {
	if (mWordWrap) delete mWordWrap;
}


int GVFileThumbnailViewItem::availableTextWidth() const {
	GVFileThumbnailView *view=static_cast<GVFileThumbnailView*>(iconView());
	if (iconView()->itemTextPos()==QIconView::Right) {
		// -2 because of the pixmap margin, see calcRect()
		return view->gridX() - view->thumbnailSize() - view->marginSize() - 2;
	} else {
		return view->thumbnailSize();
	}
}


void GVFileThumbnailViewItem::updateInfoLines() {
	mInfoLines.clear();
	if (!mFileItem) return;
	if (iconView()->itemTextPos()==QIconView::Right) {
		if (!mFileItem->isDir()) {
			mInfoLines << KIO::convertSize(mFileItem->size());
		}
		mInfoLines << mFileItem->timeString();
	}
	if (mImageSize.isValid()) {
		mInfoLines << QString::number(mImageSize.width())+"x"+QString::number(mImageSize.height());
	}
	calcRect();
}


void GVFileThumbnailViewItem::calcRect(const QString& text_) {
	GVFileThumbnailView *view=static_cast<GVFileThumbnailView*>(iconView());
	Q_ASSERT(view);
	if (!view) return;
	Q_ASSERT(pixmap());
	if (!pixmap()) return;

	int thumbnailSize=view->thumbnailSize();
	QFontMetrics fm = view->fontMetrics();

	int maxTextWidth=availableTextWidth();
	int textWidth=0;
	int textHeight=fm.height();
	
	if (iconView()->wordWrapIconText() && iconView()->itemTextPos()==QIconView::Bottom) {
		// Word-wrap up to 3 lines if the text is on bottom
		textHeight*=3;
	}
	// When is text_ set ? Doesn't look like it's ever set.
	QString txt = text_.isEmpty() ? text() : text_;

	if (mWordWrap) delete mWordWrap;
	mWordWrap=KWordWrap::formatText(fm, QRect(0, 0, maxTextWidth, textHeight),
		(view->itemTextPos()==QIconView::Bottom ? AlignHCenter : AlignAuto)
		| WordBreak, txt );
	
	textWidth=QMIN(mWordWrap->boundingRect().width(), maxTextWidth);
	textHeight=mWordWrap->boundingRect().height();
	
	// Take info lines width into account
	// Find maximum width of info lines, truncate them if they are too long
	QValueListIterator<QString> it=mInfoLines.begin();
	QValueListIterator<QString> itEnd=mInfoLines.end();
	for (;it!=itEnd; ++it) {
		int lineWidth=fm.width(*it);
		if (lineWidth>maxTextWidth) {
			*it=truncateText(fm, *it, maxTextWidth);
			// No need to go further, we just reached the max
			textWidth=maxTextWidth;
			break;
		}
		textWidth=QMAX(textWidth, lineWidth);
	}

	// Append the info lines height
	textHeight+=mInfoLines.count() * fm.height();
	
	if (view->itemTextPos() == QIconView::Right) {
		// Make sure we don't get longer than the icon height
		if (textHeight > thumbnailSize) {
			textHeight=thumbnailSize;
		}
	}

	// Center rects
	QRect itemIconRect = QRect(0,0,thumbnailSize+2,thumbnailSize+2);
	QRect itemRect = rect();
	QRect itemTextRect;
	
	int w = 0;
	int h = 0;
	if ( view->itemTextPos() == QIconView::Bottom ) {
		w = QMAX( textWidth, itemIconRect.width() );
		h = textHeight + itemIconRect.height() + 1;

		itemRect.setWidth( w );
		itemRect.setHeight( h );
		int width = QMAX( w, QApplication::globalStrut().width() ); // see QIconViewItem::width()
		int height = QMAX( h, QApplication::globalStrut().height() ); // see QIconViewItem::height()
		itemTextRect = QRect( ( width - textWidth ) / 2, height - textHeight,
							textWidth, textHeight);

		itemIconRect = QRect( ( width - itemIconRect.width() ) / 2, 0,
							itemIconRect.width(), itemIconRect.height() );
	} else {
		h = QMAX( textHeight, itemIconRect.height() );
		w = textWidth + itemIconRect.width() + 1;

		itemRect.setWidth( w );
		itemRect.setHeight( h );
		int width = QMAX( w, QApplication::globalStrut().width() ); // see QIconViewItem::width()
		int height = QMAX( h, QApplication::globalStrut().height() ); // see QIconViewItem::height()

		itemTextRect = QRect( width - textWidth, ( height - textHeight ) / 2,
							textWidth, textHeight );
		if ( itemIconRect.height() > textHeight ) // icon bigger than text -> center vertically
			itemIconRect = QRect( 0, ( height - itemIconRect.height() ) / 2,
								itemIconRect.width(), itemIconRect.height() );
		else // icon smaller than text -> center with first line
			itemIconRect = QRect( 0, ( fm.height() - itemIconRect.height() ) / 2,
								itemIconRect.width(), itemIconRect.height() );
	}

	// Apply margin on Y axis
	int margin=view->marginSize();
	itemIconRect.moveBy(0,margin/2);
	itemTextRect.moveBy(0,margin/2);
	itemRect.setHeight(itemRect.height()+margin);


	// Update rects
	if ( itemIconRect != pixmapRect() )
		setPixmapRect( itemIconRect );
	if ( itemTextRect != textRect() )
		setTextRect( itemTextRect );
	if ( itemRect != rect() )
		setItemRect( itemRect );
}


void GVFileThumbnailViewItem::paintItem(QPainter *p, const QColorGroup &cg) {
	GVFileThumbnailView *view=static_cast<GVFileThumbnailView*>(iconView());
	Q_ASSERT(view);
	if (!view) return;

	p->save();

	// Get the rects
	QRect pRect=pixmapRect(false);
	QRect tRect=textRect(false);

	// Draw pixmap
	p->drawPixmap( pRect.x()+1, pRect.y()+1, *pixmap() );

	// Draw focus
	if ( isSelected() ) {
		p->setPen( QPen( cg.highlight() ) );
		QRect outerRect=pRect | tRect;
		

		p->drawRect(outerRect);
		if (view->itemTextPos()==QIconView::Bottom) {
			p->fillRect( outerRect.x(),tRect.y(),outerRect.width(),tRect.height(), cg.highlight() );
		} else {
			p->fillRect( tRect.x(),outerRect.y(),tRect.width(),outerRect.height(), cg.highlight() );
		}

		p->setPen( QPen( cg.highlightedText() ) );
		p->setBackgroundColor(cg.highlight());
	} else {
		if ( view->itemTextBackground() != NoBrush ) {
			p->fillRect( tRect, view->itemTextBackground() );
		}
		p->setBackgroundColor(cg.base());
		p->setPen( cg.text() );
	}

	// Draw text. We draw the info lines first, because mWordWrap might alter
	// the painter settings
	int align = view->itemTextPos() == QIconView::Bottom ? AlignHCenter : AlignAuto;
	align|=AlignTop;
	if (view->shownFileItem() && view->shownFileItem()->extraData(view)==this) {
		p->setPen(view->shownFileItemColor());
	}
	if (!mWordWrap) {
		kdWarning() << "KIconViewItem::paintItem called but wordwrap not ready - calcRect not called, or aborted!" << endl;
		return;
	}
	if (mInfoLines.count() > 0) {
		int mainHeight=mWordWrap->boundingRect().height();
		QRect rect(
			tRect.x(),
			tRect.y() + mainHeight,
			view->itemTextPos() == QIconView::Bottom ? tRect.width() : availableTextWidth(),
			tRect.height() - mainHeight);
		p->drawText(rect, align, mInfoLines.join("\n"));
	}
	
	mWordWrap->drawText( p, tRect.x(), tRect.y(), align | KWordWrap::FadeOut);
	p->restore();
}


bool GVFileThumbnailViewItem::acceptDrop(const QMimeSource* source) const {
	return KURLDrag::canDecode(source);
}


void GVFileThumbnailViewItem::dropped(QDropEvent* event, const QValueList<QIconDragItem>&) {
	GVFileThumbnailView *view=static_cast<GVFileThumbnailView*>(iconView());
	emit view->dropped(event,mFileItem);
}

void GVFileThumbnailViewItem::setImageSize(const QSize& size) {
	mImageSize=size;
	updateInfoLines();
}
