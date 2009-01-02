// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>
Copyright 2008 Ilya Konkov <eruart@gmail.com>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef THUMBNAILBARVIEW_H
#define THUMBNAILBARVIEW_H

// Qt
#include <QAbstractItemDelegate>

// KDE

// Local
#include <lib/thumbnailview/thumbnailview.h>

class QTimeLine;

namespace Gwenview {


struct ThumbnailBarItemDelegatePrivate;

class ThumbnailBarItemDelegate : public QAbstractItemDelegate {
	Q_OBJECT
public:
	ThumbnailBarItemDelegate(ThumbnailView*);
	~ThumbnailBarItemDelegate();

	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
	virtual QSize sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const;


protected:
	virtual bool eventFilter(QObject*, QEvent*);

private:
	ThumbnailBarItemDelegatePrivate* const d;
	friend struct ThumbnailBarItemDelegatePrivate;
};


class ThumbnailBarView : public ThumbnailView {
	Q_OBJECT
public:
	ThumbnailBarView(QWidget* = 0);
	~ThumbnailBarView();

protected:
	void paintEvent(QPaintEvent*);
	virtual void showEvent(QShowEvent* event);

	virtual void resizeEvent(QResizeEvent * event);
	virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
	QStyle* mStyle;
	QTimeLine* mTimeLine;

	void smoothScrollTo(const QModelIndex& index);
	int horizontalScrollToValue(const QRect& rect);
};

} // namespace

#endif /* THUMBNAILBARVIEW_H */
