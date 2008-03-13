// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef PREVIEWITEMDELEGATE_H
#define PREVIEWITEMDELEGATE_H

// Qt
#include <QAbstractItemDelegate>

// KDE

// Local

namespace Gwenview {


class ThumbnailView;


class PreviewItemDelegatePrivate;

/**
 * An ItemDelegate which generates thumbnails for images. It also makes sure
 * all items are of the same size.
 */
class PreviewItemDelegate : public QAbstractItemDelegate {
	Q_OBJECT
public:
	PreviewItemDelegate(ThumbnailView*);
	~PreviewItemDelegate();

	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
	virtual QSize sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const;

	QModelIndex indexUnderCursor() const;

private Q_SLOTS:
	void setThumbnailSize(int);
	void updateButtonFrameOpacity();

protected:
	virtual bool eventFilter(QObject*, QEvent*);

private:
	PreviewItemDelegatePrivate* const d;
	friend class PreviewItemDelegatePrivate;
};


} // namespace

#endif /* PREVIEWITEMDELEGATE_H */
