// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>
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

#include <lib/gwenviewlib_export.h>

// Qt
#include <QAbstractItemDelegate>

// KF

// Local
#include <lib/thumbnailview/thumbnailview.h>

namespace Gwenview
{
struct ThumbnailBarItemDelegatePrivate;

class GWENVIEWLIB_EXPORT ThumbnailBarItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    ThumbnailBarItemDelegate(ThumbnailView *);
    ~ThumbnailBarItemDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const override;

protected:
    bool eventFilter(QObject *, QEvent *) override;

private Q_SLOTS:
    void toggleSelection();

private:
    ThumbnailBarItemDelegatePrivate *const d;
    friend struct ThumbnailBarItemDelegatePrivate;
};

struct ThumbnailBarViewPrivate;
class GWENVIEWLIB_EXPORT ThumbnailBarView : public ThumbnailView
{
    Q_OBJECT
public:
    ThumbnailBarView(QWidget * = nullptr);
    ~ThumbnailBarView() override;

    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation);

    int rowCount() const;
    void setRowCount(int);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private Q_SLOTS:
    void slotFrameChanged(int);

private:
    ThumbnailBarViewPrivate *const d;
};

} // namespace

#endif /* THUMBNAILBARVIEW_H */
