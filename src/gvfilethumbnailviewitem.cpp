// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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


GVFileThumbnailViewItem::GVFileThumbnailViewItem(QIconView* view,const QString& text,const QPixmap& icon, KFileItem* fileItem)
: QIconViewItem(view,text,icon), mFileItem(fileItem) {
	updateLines();
	calcRect();
}


GVFileThumbnailViewItem::~GVFileThumbnailViewItem() {
}


void GVFileThumbnailViewItem::updateLines() {
	mLines.clear();
	if (!mFileItem) return;
	
	QString imageSizeLine;
	if (mImageSize.isValid()) {
		imageSizeLine=QString::number(mImageSize.width())+"x"+QString::number(mImageSize.height());
	}
	
	bool isDir=mFileItem->isDir();
	bool isRight=iconView()->itemTextPos()==QIconView::Right;
	
	if (isRight) {
		mLines.append(mFileItem->name());
		mLines.append( mFileItem->timeString() );
		if (!imageSizeLine.isNull()) {
			mLines.append(imageSizeLine);
		}
		if (!isDir) {
			mLines.append(KIO::convertSize(mFileItem->size()));
		}
	} else {
		mLines.append(mFileItem->name());
		if (!isDir) {
			QString line=KIO::convertSize(mFileItem->size());
			if (!imageSizeLine.isNull()) {
				line.prepend(imageSizeLine + " - ");
			}
			mLines.append(line);
		}
	}
}


void GVFileThumbnailViewItem::calcRect(const QString&) {
	QRect itemRect(x(), y(), iconView()->gridX(), iconView()->gridY());

	QRect itemPixmapRect(0, 0, iconView()->gridX(), iconView()->gridY());
	QRect itemTextRect(itemPixmapRect);
	
	// Update rects
	if ( itemPixmapRect != pixmapRect() )
		setPixmapRect( itemPixmapRect );
	if ( itemTextRect != textRect() )
		setTextRect( itemTextRect );
	if ( itemRect != rect() )
		setItemRect( itemRect );
}


void GVFileThumbnailViewItem::paintItem(QPainter *p, const QColorGroup &cg) {
	GVFileThumbnailView *view=static_cast<GVFileThumbnailView*>(iconView());
	Q_ASSERT(view);
	if (!view) return;

	QRect rt=rect();
	bool isRight=view->itemTextPos()==QIconView::Right;
	bool isShownItem=view->shownFileItem() && view->shownFileItem()->extraData(view)==this;
	int textX, textY, textW, textH;

	if (isRight) {
		textX= rt.x() + PADDING*2 + view->thumbnailSize();
		textY= rt.y() + PADDING;
		textW= rt.width() - PADDING*3 - view->thumbnailSize();
		textH= rt.height() - PADDING*2;
	} else {
		textX= rt.x() + PADDING;
		textY= rt.y() + PADDING*2 + view->thumbnailSize();
		textW= rt.width() - PADDING*2;
		textH= rt.height() - PADDING*3 - view->thumbnailSize();
	}
	textW-=SHADOW;
	textY-=SHADOW;

	// Draw pixmap
	if (isRight) {
		p->drawPixmap( rt.x() + PADDING, rt.y() + PADDING, *pixmap() );
	} else {
		p->drawPixmap(
			rt.x() + (rt.width() - pixmap()->width())/2,
			rt.y() + PADDING, 
			*pixmap() );
	}

	// Define colors
	QColor bg, fg;
	if ( isSelected() ) {
		bg=cg.highlight();
		fg=cg.highlightedText();
	} else {
		bg=cg.button();
		fg=cg.text();
	}
	if (isShownItem) {
		bg=view->shownFileItemColor();
	}
	
	// Draw frame
	p->setPen(bg);
	QRect frmRect(rt);
	frmRect.addCoords(0, 0, -SHADOW, -SHADOW);
	p->drawRect(frmRect);
	if (isSelected()) {
		if (isRight) {
			p->fillRect(
				textX,
				frmRect.y(),
				textW + PADDING,
				textH + PADDING*2,
				bg);
		} else {
			p->fillRect(
				frmRect.x(),
				textY,
				textW + PADDING*2,
				textH + PADDING,
				bg);
		}
	}
	
	// Draw shadow
	p->setPen(cg.mid());
	frmRect.moveBy(SHADOW, SHADOW);
	p->drawLine(frmRect.topRight(), frmRect.bottomRight() );
	p->drawLine(frmRect.bottomLeft(), frmRect.bottomRight() );
	
	// Draw text
	p->setPen(QPen(fg));
	p->setBackgroundColor(bg);

	int align = (isRight ? AlignAuto : AlignHCenter) | AlignTop;

	int lineHeight=view->fontMetrics().height();
	int ascent=view->fontMetrics().ascent();
	
	int ypos=0;
	if (isRight) {
		ypos=(textH - int(mLines.count())*lineHeight) / 2;
		if (ypos<0) ypos=0;
	}

	// Set up the background color for KWordWrap::drawFadeoutText()
	if (isSelected()) {
		p->setBackgroundColor(bg);
	} else {
		p->setBackgroundColor(cg.base());
	}
	
	QValueVector<QString>::ConstIterator it=mLines.begin();
	QValueVector<QString>::ConstIterator itEnd=mLines.end();
	for (;it!=itEnd && ypos + ascent<=textH; ++it, ypos+=lineHeight) {
		int length=view->fontMetrics().width(*it);
		if (length>textW) {
			p->save();
			KWordWrap::drawFadeoutText(p,
				textX,
				textY + ypos + ascent,
				textW,
				*it);
			p->restore();
		} else {
			p->drawText(
				textX,
				textY + ypos,
				textW,
				lineHeight,
				align,
				*it);
		}
	}
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
	updateLines();
}
