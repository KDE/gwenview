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
#include "archive.h"
#include "filethumbnailview.h"
#include "filethumbnailviewitem.h"
#include "fileviewconfig.h"
#include "timeutils.h"

namespace Gwenview {

const int SHOWN_ITEM_INDICATOR_SIZE = 8;

#if 0
static void printRect(const QString& txt,const QRect& rect) {
	kdWarning() << txt << " : " << rect.x() << "x" << rect.y() << " " << rect.width() << "x" << rect.height() << endl;
}
#endif


/**
 * An helper class to handle a caption line and help drawing it
 */
class FileThumbnailViewItem::Line {
protected:
	const QIconViewItem* mItem;
	QString mTxt;
	int mWidth;
public:
	Line(const QIconViewItem* item, const QString& txt)
	: mItem(item)
	, mTxt(txt)
	, mWidth(-1) {
	}
	virtual ~Line() {}

	virtual void setWidth(int width) {
		mWidth=width;
	}
	
	virtual int height() const=0;
	
	void paint(QPainter* p, int textX, int textY, int align) const {
		Q_ASSERT(mWidth!=-1);
		int length=fontMetrics().width(mTxt);
		if (length<=mWidth ) {
			p->drawText(
				textX,
				textY,
				mWidth,
				fontMetrics().height(),
				align,
				mTxt);
		} else {
			p->save();
			complexPaint(p, textX, textY, align);
			p->restore();
		}
	};

protected:
	const FileThumbnailView* view() const {
		return static_cast<const FileThumbnailView*>(mItem->iconView());
	}

	QFontMetrics fontMetrics() const {
		return view()->fontMetrics();
	}

	/**
	 * Called when the text won't fit the available space
	 */
	virtual void complexPaint(QPainter* p, int textX, int textY, int align) const=0;
};


/**
 * A line which will get cropped if necessary
 */
class FileThumbnailViewItem::CroppedLine : public FileThumbnailViewItem::Line {
public:
	CroppedLine(const QIconViewItem* item, const QString& txt)
	: Line(item, txt) {}

	int height() const {
		return fontMetrics().height();
	}

	void complexPaint(QPainter* p, int textX, int textY, int /*align*/) const {
		KWordWrap::drawFadeoutText(p,
			textX,
			textY + fontMetrics().ascent(),
			mWidth,
			mTxt);
	}
};

/**
 * A line which will get wrapped if necessary
 */

class FileThumbnailViewItem::WrappedLine : public FileThumbnailViewItem::Line {
	KWordWrap* mWordWrap;
public:
	WrappedLine(const QIconViewItem* item, const QString& txt)
	: Line(item, txt)
	, mWordWrap(0) {}

	~WrappedLine() {
		delete mWordWrap;
	}

	int height() const {
		Q_ASSERT(mWordWrap);
		if (!mWordWrap) return 0;
		return mWordWrap->boundingRect().height();
	}

	/**
	 * Regenerates mWordWrap if the width has changed
	 */
	void setWidth(int width) {
		if (width==mWidth) return;
		mWidth=width;
		delete mWordWrap;
		QFontMetrics fm=fontMetrics();
		mWordWrap=KWordWrap::formatText(fm,
			QRect(0, 0, mWidth, fm.height()*3),
			0 /*flags*/,
			mTxt);
	}

	void complexPaint(QPainter* p, int textX, int textY, int align) const {
		Q_ASSERT(mWordWrap);
		if (!mWordWrap) return;

		int xpos=0;
		if (align & AlignHCenter) {
			xpos=( mWidth - mWordWrap->boundingRect().width() ) / 2;
		}
		
		mWordWrap->drawText(p, 
			textX + xpos,
			textY,
			align);
	}
};


FileThumbnailViewItem::FileThumbnailViewItem(QIconView* view,const QString& text,const QPixmap& icon, KFileItem* fileItem)
: QIconViewItem(view,text,icon), mFileItem(fileItem) {
	updateLines();
	calcRect();
}


FileThumbnailViewItem::~FileThumbnailViewItem() {
	QValueVector<Line*>::ConstIterator it=mLines.begin();
	QValueVector<Line*>::ConstIterator itEnd=mLines.end();
	for (;it!=itEnd; ++it) {
		delete *it;
	}
}


void FileThumbnailViewItem::updateLines() {
	QValueVector<Line*>::ConstIterator it=mLines.begin();
	QValueVector<Line*>::ConstIterator itEnd=mLines.end();
	for (;it!=itEnd; ++it) {
		delete *it;
	}
	mLines.clear();
	if (!mFileItem) return;

	bool isDir=mFileItem->isDir();
	if (iconView()->itemTextPos()==QIconView::Right) {
		// Text is on the right, show everything

		time_t time = TimeUtils::getTime(mFileItem);
		mLines.append( new WrappedLine(this, mFileItem->name()) );
		mLines.append( new CroppedLine(this, TimeUtils::formatTime(time)) );
		if (mImageSize.isValid()) {
			QString txt=QString::number(mImageSize.width())+"x"+QString::number(mImageSize.height());
			mLines.append( new CroppedLine(this, txt) );
		}
		if (!isDir) {
			mLines.append( new CroppedLine(this, KIO::convertSize(mFileItem->size())) );
		}

	} else {
		// Text is below the icon, only show details selected in
		// view->itemDetails()
		FileThumbnailView *view=static_cast<FileThumbnailView*>(iconView());
		int details=view->itemDetails();
		bool isImage=!Archive::fileItemIsDirOrArchive(mFileItem);
		
		if (!isImage || (details & FileThumbnailView::FILENAME)) {
			mLines.append( new WrappedLine(this, mFileItem->name()) );
		}
		if (details & FileThumbnailView::FILEDATE) {
			time_t time = TimeUtils::getTime(mFileItem);
			mLines.append( new CroppedLine(this, TimeUtils::formatTime(time)) );
		}
		if (details & FileThumbnailView::IMAGESIZE) {
			QString txt;
			if (mImageSize.isValid()) {
				txt=QString::number(mImageSize.width())+"x"+QString::number(mImageSize.height());
			}
			mLines.append( new CroppedLine(this, txt) );
		}
		if (!isDir && (details & FileThumbnailView::FILESIZE)) {
			mLines.append( new CroppedLine(this, KIO::convertSize(mFileItem->size())) );
		}

	}

	calcRect();
}


void FileThumbnailViewItem::calcRect(const QString&) {
	FileThumbnailView *view=static_cast<FileThumbnailView*>(iconView());
	bool isRight=view->itemTextPos()==QIconView::Right;
	
	int textW=view->gridX();
	int thumbnailSize=FileViewConfig::thumbnailSize();
	if (isRight) {
		textW-=PADDING * 3 + thumbnailSize;
	} else {
		textW-=PADDING * 2;
	}
	
	int textH=0;
	QValueVector<Line*>::ConstIterator it=mLines.begin();
	QValueVector<Line*>::ConstIterator itEnd=mLines.end();
	for (;it!=itEnd; ++it) {
		(*it)->setWidth(textW);
		textH+=(*it)->height();
	}
	
	QRect itemRect(x(), y(), view->gridX(), 0);
	QRect itemPixmapRect(PADDING, PADDING, thumbnailSize, thumbnailSize);
	QRect itemTextRect(0, 0, textW, textH);
	if (isRight) {
		itemRect.setHeight( QMAX(thumbnailSize + PADDING*2, textH) );
		itemTextRect.moveLeft(thumbnailSize + PADDING * 2 );
		itemTextRect.moveTop((itemRect.height() - textH)/2);
	} else {
		itemPixmapRect.moveLeft( (itemRect.width() - itemPixmapRect.width()) / 2 );
		itemRect.setHeight(thumbnailSize + PADDING*3 + textH);
		itemTextRect.moveLeft(PADDING);
		itemTextRect.moveTop(thumbnailSize + PADDING * 2);
	}
	
	// Update rects
	if ( itemPixmapRect != pixmapRect() ) {
		setPixmapRect( itemPixmapRect );
	}
	if ( itemTextRect != textRect() ) {
		setTextRect( itemTextRect );
	}
	if ( itemRect != rect() ) {
		setItemRect( itemRect );
	}
}


void FileThumbnailViewItem::paintItem(QPainter *p, const QColorGroup &cg) {
	FileThumbnailView *view=static_cast<FileThumbnailView*>(iconView());
	Q_ASSERT(view);
	if (!view) return;

	bool isRight=view->itemTextPos()==QIconView::Right;
	bool isShownItem=view->shownFileItem() && view->shownFileItem()->extraData(view)==this;
	bool isImage=!Archive::fileItemIsDirOrArchive(mFileItem);
	int textX, textY, textW, textH;
	int thumbnailSize=FileViewConfig::thumbnailSize();

	textX=textRect(false).x();
	textY=textRect(false).y();
	textW=textRect(false).width();
	textH=textRect(false).height();

	// Draw pixmap
	QRect pRect = pixmapRect(false);
	int pixX = pRect.left() + ( thumbnailSize - pixmap()->width() ) / 2;
	int pixY = pRect.top() + ( thumbnailSize - pixmap()->height() ) / 2;
	p->drawPixmap( pixX, pixY, *pixmap() );

	QColor bg;
	if ( isSelected() ) {
		bg=cg.highlight();
	} else {
		bg=cg.mid();
	}
	
	// Draw shown item indicator
	if (isShownItem) {
		QPointArray pa(3);
		pa[0] = pixmapRect(false).bottomLeft();
		pa[0].rx() += pixmapRect(false).width() / 2;
		pa[0].ry() += PADDING - 1;
		pa[0].ry() -= SHOWN_ITEM_INDICATOR_SIZE;
		
		pa[1] = pa[0];
		pa[1].rx() -= SHOWN_ITEM_INDICATOR_SIZE;
		pa[1].ry() += SHOWN_ITEM_INDICATOR_SIZE;

		pa[2] = pa[1];
		pa[2].rx() += SHOWN_ITEM_INDICATOR_SIZE * 2;
		
		p->setBrush(cg.highlight());
		p->setPen(cg.base());
		p->drawPolygon(pa);
	}
	
	if (isImage || isSelected()) {
		// Draw frame
		QRect frmRect=pixmapRect(false);
		frmRect.addCoords(-PADDING, -PADDING, PADDING, PADDING);
		
		p->setBrush(QBrush());
		p->setPen(bg);
		p->drawRect(frmRect);
		if (isSelected()) {
			frmRect.addCoords(1, 1, -1, -1);
			p->drawRect(frmRect);
		}
	}

	// Draw text
	p->setPen(cg.text());
	p->setBackgroundColor(cg.base());
	int align = (isRight ? AlignAuto : AlignHCenter) | AlignTop;
	
	QValueVector<Line*>::ConstIterator it=mLines.begin();
	QValueVector<Line*>::ConstIterator itEnd=mLines.end();
	for (;it!=itEnd; ++it) {
		const Line* line=*it;
		line->paint(p, textX, textY, align);
		textY+=line->height();
	}
}


bool FileThumbnailViewItem::acceptDrop(const QMimeSource* source) const {
	return KURLDrag::canDecode(source);
}


void FileThumbnailViewItem::dropped(QDropEvent* event, const QValueList<QIconDragItem>&) {
	FileThumbnailView *view=static_cast<FileThumbnailView*>(iconView());
	emit view->dropped(event,mFileItem);
}

void FileThumbnailViewItem::setImageSize(const QSize& size) {
	mImageSize=size;
	updateLines();
}

} // namespace
