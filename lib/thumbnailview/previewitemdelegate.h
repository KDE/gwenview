// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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

#include <lib/gwenviewlib_export.h>

// Qt
#include <QAbstractItemDelegate>

// KDE

// Local

class KUrl;

namespace Gwenview {


class ThumbnailView;


class PreviewItemDelegatePrivate;

/**
 * An ItemDelegate which generates thumbnails for images. It also makes sure
 * all items are of the same size.
 */
class GWENVIEWLIB_EXPORT PreviewItemDelegate : public QAbstractItemDelegate {
	Q_OBJECT
public:
	PreviewItemDelegate(ThumbnailView*);
	~PreviewItemDelegate();

	enum ThumbnailDetail {
		FileNameDetail = 1,
		DateDetail     = 2,
		RatingDetail   = 4
	};
	// FIXME: Find out why this cause problems with Qt::Alignment in
	// PreviewItemDelegate!
	Q_DECLARE_FLAGS(ThumbnailDetails, ThumbnailDetail)

	/**
	 * Returns which thumbnail details are shown
	 */
	ThumbnailDetails thumbnailDetails() const;

	void setThumbnailDetails(ThumbnailDetails);

	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
	virtual QSize sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const;

Q_SIGNALS:
	void saveDocumentRequested(const KUrl&);
	void rotateDocumentLeftRequested(const KUrl&);
	void rotateDocumentRightRequested(const KUrl&);
	void showDocumentInFullScreenRequested(const KUrl&);
	void setDocumentRatingRequested(const KUrl&, int rating);

private Q_SLOTS:
	void setThumbnailSize(int);

	void slotSaveClicked();
	void slotRotateLeftClicked();
	void slotRotateRightClicked();
	void slotFullScreenClicked();
	void slotToggleSelectionClicked();

protected:
	virtual bool eventFilter(QObject*, QEvent*);

private:
	PreviewItemDelegatePrivate* const d;
	friend class PreviewItemDelegatePrivate;
};


} // namespace

// See upper
Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::PreviewItemDelegate::ThumbnailDetails)

#endif /* PREVIEWITEMDELEGATE_H */
