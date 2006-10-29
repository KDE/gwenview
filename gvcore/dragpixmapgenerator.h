// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#ifndef DRAGPIXMAPGENERATOR_H
#define DRAGPIXMAPGENERATOR_H

// Qt
#include <qapplication.h>
#include <qpixmap.h>
#include <qtooltip.h>
#include <qvaluelist.h>
#include <qvaluevector.h>

// KDE
#include <klocale.h>

namespace Gwenview {


template <class T>
class DragPixmapGenerator;


template <class T>
class DragPixmapItemDrawer {
public:
	DragPixmapItemDrawer()
	: mGenerator(0) {}
	virtual ~DragPixmapItemDrawer() {}
	virtual void setGenerator(DragPixmapGenerator<T>* generator) {
		mGenerator = generator;
	}
	virtual QSize itemSize(T)=0;
	virtual void drawItem(QPainter*, int left, int top, T)=0;
	virtual int spacing() const { 
		return 0;
	}

protected:
	DragPixmapGenerator<T>* mGenerator;
};


QPixmap dragPixmapGeneratorHelper(QValueVector<QPixmap> pixmapVector);


template <class T>
class DragPixmapGenerator {
public:
	/** Offset between cursor and dragged images */
	static const uint DRAG_OFFSET=16;


	/** Maximum width of an item painted by DragPixmapItemDrawer */
	static const int ITEM_MAX_WIDTH=128;

	static const int MAX_HEIGHT=200;

	static const int DRAG_MARGIN=4;


	DragPixmapGenerator()
	: mPixmapWidth(0) {}


    void addItem(const T& item) {
		mItemList << item;
	}


	int maxWidth() const {
		return ITEM_MAX_WIDTH;
	}


	/**
	 * Returns the width of the generated pixmap, not including the margin.
	 * To be used by DragPixmapItemDrawer<T>::drawItem. Should not be used
	 * anywhere else since this value is initialized in generate().
	 */
	int pixmapWidth() const {
		return mPixmapWidth;
	}

    void setItemDrawer(DragPixmapItemDrawer<T>* drawer) {
		mItemDrawer = drawer;
		drawer->setGenerator(this);
	}

    QPixmap generate() {
		int width = 0, height = 0;
		int dragCount = 0;
		int spacing = mItemDrawer->spacing();
		bool listCropped;
		QString bottomText;
		QFontMetrics fm = QApplication::fontMetrics();

		// Compute pixmap size and update dragCount
		QValueListIterator<T> it = mItemList.begin();
		QValueListIterator<T> end = mItemList.end();
		height = -spacing;
		for (; it!= end && height < MAX_HEIGHT; ++dragCount, ++it) {
			QSize itemSize = mItemDrawer->itemSize(*it);
			Q_ASSERT(itemSize.width() <= ITEM_MAX_WIDTH);
			
			width = QMAX(width, itemSize.width());
			height += itemSize.height() + spacing;
		}

		listCropped = it != end;
		if (listCropped) {
			// If list has been cropped, leave space for item count text
			height += fm.height();
			bottomText = i18n("%1 items").arg(mItemList.count());
			width = QMAX(width, fm.width("... " + bottomText));
		}
		
		mPixmapWidth = width;

		// Init pixmap
		QPixmap pixmap(width + 2*DRAG_MARGIN, height + 2*DRAG_MARGIN);
		QColorGroup cg = QToolTip::palette().active();

		pixmap.fill(cg.base());
		QPainter painter(&pixmap);
		
		// Draw border
		painter.setPen(cg.dark());
		painter.drawRect(pixmap.rect());
		
		// Draw items
		it = mItemList.begin();
		height = DRAG_MARGIN;
		for (int pos=0; pos < dragCount; ++pos, ++it) {
			mItemDrawer->drawItem(&painter, DRAG_MARGIN, height, *it);
			height += mItemDrawer->itemSize(*it).height() + spacing;
		}

		// Draw text if necessary
		if (listCropped) {
			int posY= height + fm.ascent();
			painter.drawText(DRAG_MARGIN, posY, "...");
			int offset = width - fm.width(bottomText);
			painter.drawText(DRAG_MARGIN + offset, posY, bottomText);
		}
		painter.end();

		return pixmap;
	}


private:
	QValueList<T> mItemList;
	DragPixmapItemDrawer<T>* mItemDrawer;
	int mPixmapWidth;
};


} // namespace


#endif /* DRAGPIXMAPGENERATOR_H */
