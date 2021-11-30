/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef THUMBNAILVIEW_H
#define THUMBNAILVIEW_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QListView>
#include <QUrl>

// KF

class KFileItem;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QPixmap;

namespace Gwenview
{
class AbstractDocumentInfoProvider;
class AbstractThumbnailViewHelper;
class ThumbnailProvider;

struct ThumbnailViewPrivate;
class GWENVIEWLIB_EXPORT ThumbnailView : public QListView
{
    Q_OBJECT
public:
    enum {
        MinThumbnailSize = 48,
        MaxThumbnailSize = 1024,
    };

    enum ThumbnailScaleMode {
        ScaleToSquare,
        ScaleToHeight,
        ScaleToWidth,
        ScaleToFit,
    };
    explicit ThumbnailView(QWidget *parent);
    ~ThumbnailView() override;

    void setThumbnailViewHelper(AbstractThumbnailViewHelper *helper);

    AbstractThumbnailViewHelper *thumbnailViewHelper() const;

    void setDocumentInfoProvider(AbstractDocumentInfoProvider *provider);

    AbstractDocumentInfoProvider *documentInfoProvider() const;

    ThumbnailScaleMode thumbnailScaleMode() const;

    void setThumbnailScaleMode(ThumbnailScaleMode);

    /**
     * Returns the thumbnail size.
     */
    QSize thumbnailSize() const;

    /**
     * Returns the aspect ratio of the thumbnail.
     */
    qreal thumbnailAspectRatio() const;

    QPixmap thumbnailForIndex(const QModelIndex &, QSize *fullSize = nullptr);

    /**
     * Returns true if the document pointed by the index has been modified
     * inside Gwenview.
     */
    bool isModified(const QModelIndex &) const;

    /**
     * Returns true if the document pointed by the index is currently busy
     * (loading, saving, rotating...)
     */
    bool isBusy(const QModelIndex &index) const;

    void setModel(QAbstractItemModel *model) override;

    void setThumbnailProvider(ThumbnailProvider *thumbnailProvider);

    /**
     * Publish this method so that delegates can call it.
     */
    using QListView::scheduleDelayedItemsLayout;

    /**
     * Returns the current pixmap to paint when drawing a busy index.
     */
    QPixmap busySequenceCurrentPixmap() const;

    void reloadThumbnail(const QModelIndex &);

    void updateThumbnailSize();

    void setCreateThumbnailsForRemoteUrls(bool createRemoteThumbs);

Q_SIGNALS:
    /**
     * It seems we can't use the 'activated()' signal for now because it does
     * not know about KDE single vs doubleclick settings. The indexActivated()
     * signal replaces it.
     */
    void indexActivated(const QModelIndex &);

    void thumbnailSizeChanged(const QSize &);
    void thumbnailWidthChanged(int);

    /**
     * Emitted whenever selectionChanged() is called.
     * This signal is suffixed with "Signal" because
     * QAbstractItemView::selectionChanged() is a slot.
     */
    void selectionChangedSignal(const QItemSelection &, const QItemSelection &);

    /**
     * Forward some signals from model, so that the delegate can use them
     */
    void rowsRemovedSignal(const QModelIndex &parent, int start, int end);

    void rowsInsertedSignal(const QModelIndex &parent, int start, int end);

public Q_SLOTS:
    /**
     * Sets the thumbnail's width, in pixels. Keeps aspect ratio unchanged.
     */
    void setThumbnailWidth(int width);

    /**
     * Sets the thumbnail's aspect ratio. Keeps width unchanged.
     */
    void setThumbnailAspectRatio(qreal ratio);

    void scrollToSelectedIndex();

    void generateThumbnailsForItems();

protected:
    void dragEnterEvent(QDragEnterEvent *) override;

    void dragMoveEvent(QDragMoveEvent *) override;

    void dropEvent(QDropEvent *) override;

    void keyPressEvent(QKeyEvent *) override;

    void resizeEvent(QResizeEvent *) override;

    void scrollContentsBy(int dx, int dy) override;

    void showEvent(QShowEvent *) override;

    void wheelEvent(QWheelEvent *) override;

    void startDrag(Qt::DropActions) override;

    void mousePressEvent(QMouseEvent *) override;

protected Q_SLOTS:
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) override;
    void rowsInserted(const QModelIndex &parent, int start, int end) override;
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;

private Q_SLOTS:
    void startDragFromTouch(const QPoint &pos);
    void tapGesture(const QPoint &pos);
    void setZoomParameter();
    void zoomGesture(qreal newZoom, const QPoint &pos);
    void showContextMenu();
    void emitIndexActivatedIfNoModifiers(const QModelIndex &);
    void setThumbnail(const KFileItem &, const QPixmap &, const QSize &, qulonglong fileSize);
    void setBrokenThumbnail(const KFileItem &);

    /**
     * Generate thumbnail for url.
     */
    void updateThumbnail(const QUrl &url);

    /**
     * Tells the view the busy state of the document pointed by the url has changed.
     */
    void updateThumbnailBusyState(const QUrl &url, bool);

    /*
     * Cause a repaint of all busy indexes
     */
    void updateBusyIndexes();

    void smoothNextThumbnail();

private:
    friend struct ThumbnailViewPrivate;
    ThumbnailViewPrivate *const d;
};

} // namespace

#endif /* THUMBNAILVIEW_H */
