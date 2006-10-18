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
#include <qpixmap.h>
#include <qvaluelist.h>
#include <qvaluevector.h>

namespace Gwenview {


template <class T>
class DragPixmapProvider {
public:
	virtual ~DragPixmapProvider() {}
	virtual QPixmap pixmapForItem(T)=0;
};


QPixmap dragPixmapGeneratorHelper(QValueVector<QPixmap> pixmapVector);


template <class T>
class DragPixmapGenerator {
public:
	static const uint MAX_PIXMAP_COUNT=4;


    void addItem(const T& item) {
		mItemList << item;
	}


    void setItemPixmapProvider(DragPixmapProvider<T>* provider) {
		mProvider = provider;
	}

	
    QPixmap generate() {
		int dragCount = QMIN(mItemList.count(), MAX_PIXMAP_COUNT);
		QValueVector<QPixmap> pixmapVector;

		QValueListIterator<T> it = mItemList.begin();
		for (int pos=0; pos < dragCount; ++pos, ++it) {
			QPixmap pix = mProvider->pixmapForItem(*it);
			pixmapVector.push_back(pix);
		}

		return dragPixmapGeneratorHelper(pixmapVector);
	}


private:
	QValueList<T> mItemList;
	DragPixmapProvider<T>* mProvider;
};


} // namespace


#endif /* DRAGPIXMAPGENERATOR_H */
