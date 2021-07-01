// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include <QItemDelegate>

// KF

// Local

class QUrl;

namespace Gwenview
{
class ThumbnailView;

struct PreviewItemDelegatePrivate;

/**
 * An ItemDelegate which generates thumbnails for images. It also makes sure
 * all items are of the same size.
 */
class GWENVIEWLIB_EXPORT PreviewItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    PreviewItemDelegate(ThumbnailView *);
    ~PreviewItemDelegate() override;

    enum ContextBarAction {
        NoAction = 0,
        SelectionAction = 1,
        FullScreenAction = 2,
        RotateAction = 4,
    };
    Q_DECLARE_FLAGS(ContextBarActions, ContextBarAction)

    enum ThumbnailDetail {
        FileNameDetail = 1,
        DateDetail = 2,
        RatingDetail = 4,
        ImageSizeDetail = 8,
        FileSizeDetail = 16,
    };
    // FIXME: Find out why this cause problems with Qt::Alignment in
    // PreviewItemDelegate!
    Q_DECLARE_FLAGS(ThumbnailDetails, ThumbnailDetail)

    /**
     * Returns which thumbnail details are shown
     */
    ThumbnailDetails thumbnailDetails() const;

    void setThumbnailDetails(ThumbnailDetails);

    ContextBarActions contextBarActions() const;

    void setContextBarActions(ContextBarActions);

    Qt::TextElideMode textElideMode() const;

    void setTextElideMode(Qt::TextElideMode);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const override;

Q_SIGNALS:
    void saveDocumentRequested(const QUrl &);
    void rotateDocumentLeftRequested(const QUrl &);
    void rotateDocumentRightRequested(const QUrl &);
    void showDocumentInFullScreenRequested(const QUrl &);
    void setDocumentRatingRequested(const QUrl &, int rating);

private Q_SLOTS:
    void setThumbnailSize(const QSize &);

    void slotSaveClicked();
    void slotRotateLeftClicked();
    void slotRotateRightClicked();
    void slotFullScreenClicked();
    void slotToggleSelectionClicked();
    void slotRowsChanged();

protected:
    bool eventFilter(QObject *, QEvent *) override;

private:
    PreviewItemDelegatePrivate *const d;
    friend struct PreviewItemDelegatePrivate;
};

} // namespace

// See upper
Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::PreviewItemDelegate::ThumbnailDetails)
Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::PreviewItemDelegate::ContextBarActions)

#endif /* PREVIEWITEMDELEGATE_H */
