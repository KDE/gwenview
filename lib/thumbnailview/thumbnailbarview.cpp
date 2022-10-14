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
// Self
#include "thumbnailbarview.h"

// Qt
#include <QApplication>
#include <QHelpEvent>
#include <QItemSelectionModel>
#include <QPainter>
#include <QScrollBar>
#include <QTimeLine>
#include <QToolButton>
#include <QToolTip>

#ifdef WINDOWS_PROXY_STYLE
#include <QWindowsStyle>
#endif

// KF

// Local
#include "gwenview_lib_debug.h"
#include "gwenviewconfig.h"
#include "lib/paintutils.h"

namespace Gwenview
{
/**
 * Duration in ms of the smooth scroll
 */
const int SMOOTH_SCROLL_DURATION = 250;

/**
 * Space between the item outer rect and the content, and between the
 * thumbnail and the caption
 */
const int ITEM_MARGIN = 5;

/** How dark is the shadow, 0 is invisible, 255 is as dark as possible */
const int SHADOW_STRENGTH = 127;

/** How many pixels around the thumbnail are shadowed */
const int SHADOW_SIZE = 4;

struct ThumbnailBarItemDelegatePrivate {
    // Key is height * 1000 + width
    using ShadowCache = QMap<int, QPixmap>;
    mutable ShadowCache mShadowCache;

    ThumbnailBarItemDelegate *q = nullptr;
    ThumbnailView *mView = nullptr;
    QToolButton *mToggleSelectionButton = nullptr;

    QColor mBorderColor;
    QPersistentModelIndex mIndexUnderCursor;

    void setupToggleSelectionButton()
    {
        mToggleSelectionButton = new QToolButton(mView->viewport());
        mToggleSelectionButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        mToggleSelectionButton->hide();
        QObject::connect(mToggleSelectionButton, &QToolButton::clicked, q, &ThumbnailBarItemDelegate::toggleSelection);
    }

    void showToolTip(QHelpEvent *helpEvent)
    {
        QModelIndex index = mView->indexAt(helpEvent->pos());
        if (!index.isValid()) {
            return;
        }
        QString fullText = index.data().toString();
        QPoint pos = QCursor::pos();
        QToolTip::showText(pos, fullText, mView);
    }

    void drawShadow(QPainter *painter, const QRect &rect) const
    {
        const QPoint shadowOffset(-SHADOW_SIZE, -SHADOW_SIZE + 1);

        const auto dpr = painter->device()->devicePixelRatioF();
        int key = qRound((rect.height() * 1000 + rect.width()) * dpr);

        ShadowCache::Iterator it = mShadowCache.find(key);
        if (it == mShadowCache.end()) {
            QSize size = QSize(rect.width() + 2 * SHADOW_SIZE, rect.height() + 2 * SHADOW_SIZE);
            QColor color(0, 0, 0, SHADOW_STRENGTH);
            QPixmap shadow = PaintUtils::generateFuzzyRect(size * dpr, color, qRound(SHADOW_SIZE * dpr));
            shadow.setDevicePixelRatio(dpr);
            it = mShadowCache.insert(key, shadow);
        }
        painter->drawPixmap(rect.topLeft() + shadowOffset, it.value());
    }

    bool hoverEventFilter(QHoverEvent *event)
    {
        QModelIndex index = mView->indexAt(event->pos());
        if (index != mIndexUnderCursor) {
            updateHoverUi(index);
        }
        return false;
    }

    void updateHoverUi(const QModelIndex &index)
    {
        mIndexUnderCursor = index;

        if (mIndexUnderCursor.isValid() && GwenviewConfig::thumbnailActions() != ThumbnailActions::None) {
            updateToggleSelectionButton();

            const QRect rect = mView->visualRect(mIndexUnderCursor);
            mToggleSelectionButton->move(rect.topLeft() + QPoint(2, 2));
            mToggleSelectionButton->show();
        } else {
            mToggleSelectionButton->hide();
        }
    }

    void updateToggleSelectionButton()
    {
        bool isSelected = mView->selectionModel()->isSelected(mIndexUnderCursor);
        mToggleSelectionButton->setIcon(QIcon::fromTheme(isSelected ? QStringLiteral("list-remove") : QStringLiteral("list-add")));
    }
};

ThumbnailBarItemDelegate::ThumbnailBarItemDelegate(ThumbnailView *view)
    : QAbstractItemDelegate(view)
    , d(new ThumbnailBarItemDelegatePrivate)
{
    d->q = this;
    d->mView = view;
    d->setupToggleSelectionButton();
    view->viewport()->installEventFilter(this);

    // Set this attribute so that the viewport receives QEvent::HoverMove and
    // QEvent::HoverLeave events. We use these events in the event filter
    // installed on the viewport.
    // Some styles set this attribute themselves (Oxygen and Skulpture do) but
    // others do not (Plastique, Cleanlooks...)
    view->viewport()->setAttribute(Qt::WA_Hover);

    d->mBorderColor = PaintUtils::alphaAdjustedF(QColor(Qt::white), 0.65);

    connect(view, &ThumbnailView::selectionChangedSignal, [this]() {
        d->updateToggleSelectionButton();
    });
}

QSize ThumbnailBarItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex &index) const
{
    QSize size;
    if (d->mView->thumbnailScaleMode() == ThumbnailView::ScaleToFit) {
        size = d->mView->gridSize();
    } else {
        QPixmap thumbnailPix = d->mView->thumbnailForIndex(index);
        size = thumbnailPix.size() / thumbnailPix.devicePixelRatio();
        size.rwidth() += ITEM_MARGIN * 2;
        size.rheight() += ITEM_MARGIN * 2;
    }
    return size;
}

bool ThumbnailBarItemDelegate::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ToolTip:
        d->showToolTip(static_cast<QHelpEvent *>(event));
        return true;
    case QEvent::HoverMove:
    case QEvent::HoverLeave:
        return d->hoverEventFilter(static_cast<QHoverEvent *>(event));
    default:
        break;
    }

    return false;
}

void ThumbnailBarItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool isSelected = option.state & QStyle::State_Selected;
    bool isCurrent = d->mView->selectionModel()->currentIndex() == index;
    QPixmap thumbnailPix = d->mView->thumbnailForIndex(index);
    QSize thumbnailSize = thumbnailPix.size() / thumbnailPix.devicePixelRatio();
    QRect rect = option.rect;

    QStyleOptionViewItem opt = option;
    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    if (isSelected && !isCurrent) {
        // Draw selected but not current item backgrounds with some transparency
        // so that the current item stands out.
        painter->setOpacity(.33);
    }
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    painter->setOpacity(1);

    // Draw thumbnail
    if (!thumbnailPix.isNull()) {
        QRect thumbnailRect = QRect(rect.left() + (rect.width() - thumbnailSize.width()) / 2,
                                    rect.top() + (rect.height() - thumbnailSize.height()) / 2 - 1,
                                    thumbnailSize.width(),
                                    thumbnailSize.height());

        if (!thumbnailPix.hasAlphaChannel()) {
            d->drawShadow(painter, thumbnailRect);
            painter->setPen(d->mBorderColor);
            painter->setRenderHint(QPainter::Antialiasing, false);
            QRect borderRect = thumbnailRect.adjusted(-1, -1, 0, 0);
            painter->drawRect(borderRect);
        }
        painter->drawPixmap(thumbnailRect.left(), thumbnailRect.top(), thumbnailPix);

        // Draw busy indicator
        if (d->mView->isBusy(index)) {
            QPixmap pix = d->mView->busySequenceCurrentPixmap();
            painter->drawPixmap(thumbnailRect.left() + (thumbnailRect.width() - pix.width()) / 2,
                                thumbnailRect.top() + (thumbnailRect.height() - pix.height()) / 2,
                                pix);
        }
    }
}

void ThumbnailBarItemDelegate::toggleSelection()
{
    d->mView->selectionModel()->select(d->mIndexUnderCursor, QItemSelectionModel::Toggle);
}

ThumbnailBarItemDelegate::~ThumbnailBarItemDelegate()
{
    delete d;
}

// this is disabled by David Edmundson as I can't figure out how to port it
// I hope with breeze being the default we don't want to start making our own styles anyway
#ifdef WINDOWS_PROXY_STYLE
/**
 * This proxy style makes it possible to override the value returned by
 * styleHint() which leads to not-so-nice results with some styles.
 *
 * We cannot use QProxyStyle because it takes ownership of the base style,
 * which causes crash when user change styles.
 */
class ProxyStyle : public QWindowsStyle
{
public:
    ProxyStyle()
        : QWindowsStyle()
    {
    }

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w = 0) const
    {
        QApplication::style()->drawPrimitive(pe, opt, p, w);
    }

    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *w = 0) const
    {
        QApplication::style()->drawControl(element, opt, p, w);
    }

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w = 0) const
    {
        QApplication::style()->drawComplexControl(cc, opt, p, w);
    }

    int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = 0, QStyleHintReturn *shret = 0) const
    {
        switch (sh) {
        case SH_ItemView_ShowDecorationSelected:
            // We want the highlight to cover our thumbnail
            return true;
        case SH_ScrollView_FrameOnlyAroundContents:
            // Ensure the frame does not include the scrollbar. This ensure the
            // scrollbar touches the edge of the window and thus can touch the
            // edge of the screen when maximized
            return false;
        default:
            return QApplication::style()->styleHint(sh, opt, w, shret);
        }
    }

    void polish(QApplication *application)
    {
        QApplication::style()->polish(application);
    }

    void polish(QPalette &palette)
    {
        QApplication::style()->polish(palette);
    }

    void polish(QWidget *widget)
    {
        QApplication::style()->polish(widget);
    }

    void unpolish(QWidget *widget)
    {
        QApplication::style()->unpolish(widget);
    }

    void unpolish(QApplication *application)
    {
        QApplication::style()->unpolish(application);
    }

    int pixelMetric(PixelMetric pm, const QStyleOption *opt, const QWidget *widget) const
    {
        switch (pm) {
        case PM_MaximumDragDistance:
            // Ensure the fullscreen thumbnailbar does not go away while
            // dragging the scrollbar if the mouse cursor is too far away from
            // the widget
            return -1;
        default:
            return QApplication::style()->pixelMetric(pm, opt, widget);
        }
    }
};
#endif // WINDOWS_PROXY_STYLE

using QSizeDimension = int (QSize::*)() const;

struct ThumbnailBarViewPrivate {
    ThumbnailBarView *q;
    QStyle *mStyle;
    QTimeLine *mTimeLine;

    Qt::Orientation mOrientation;
    int mRowCount;

    QScrollBar *scrollBar() const
    {
        return mOrientation == Qt::Horizontal ? q->horizontalScrollBar() : q->verticalScrollBar();
    }

    QSizeDimension mainDimension() const
    {
        return mOrientation == Qt::Horizontal ? &QSize::width : &QSize::height;
    }

    QSizeDimension oppositeDimension() const
    {
        return mOrientation == Qt::Horizontal ? &QSize::height : &QSize::width;
    }

    void smoothScrollTo(const QModelIndex &index)
    {
        if (!index.isValid()) {
            return;
        }

        const QRect rect = q->visualRect(index);

        int oldValue = scrollBar()->value();
        int newValue = scrollToValue(rect);
        if (mTimeLine->state() == QTimeLine::Running) {
            mTimeLine->stop();
        }
        mTimeLine->setFrameRange(oldValue, newValue);
        mTimeLine->start();
    }

    int scrollToValue(const QRect &rect)
    {
        // This code is a much simplified version of
        // QListViewPrivate::horizontalScrollToValue()
        const QRect area = q->viewport()->rect();
        int value = scrollBar()->value();

        if (mOrientation == Qt::Horizontal) {
            if (q->isRightToLeft()) {
                value += (area.width() - rect.width()) / 2 - rect.left();
            } else {
                value += rect.left() - (area.width() - rect.width()) / 2;
            }
        } else {
            value += rect.top() - (area.height() - rect.height()) / 2;
        }
        return value;
    }

    void updateMinMaxSizes()
    {
        QSizeDimension dimension = oppositeDimension();
        int scrollBarSize = (scrollBar()->sizeHint().*dimension)();
        QSize minSize(0, mRowCount * 48 + scrollBarSize);
        QSize maxSize(QWIDGETSIZE_MAX, mRowCount * 256 + scrollBarSize);
        if (mOrientation == Qt::Vertical) {
            minSize.transpose();
            maxSize.transpose();
        }
        q->setMinimumSize(minSize);
        q->setMaximumSize(maxSize);
    }

    void updateThumbnailSize()
    {
        QSizeDimension dimension = oppositeDimension();
        int scrollBarSize = (scrollBar()->sizeHint().*dimension)();
        int widgetSize = (q->size().*dimension)();

        if (mRowCount > 1) {
            // Decrease widgetSize because otherwise the view sometimes wraps at
            // mRowCount-1 instead of mRowCount. Probably because gridSize *
            // mRowCount is too close to widgetSize.
            --widgetSize;
        }

        int gridWidth, gridHeight;
        if (mOrientation == Qt::Horizontal) {
            gridHeight = (widgetSize - scrollBarSize - 2 * q->frameWidth()) / mRowCount;
            gridWidth = qRound(gridHeight * q->thumbnailAspectRatio());
        } else {
            gridWidth = (widgetSize - scrollBarSize - 2 * q->frameWidth()) / mRowCount;
            gridHeight = qRound(gridWidth / q->thumbnailAspectRatio());
        }
        if (q->thumbnailScaleMode() == ThumbnailView::ScaleToFit) {
            q->setGridSize(QSize(gridWidth, gridHeight));
        }
        q->setThumbnailWidth(gridWidth - ITEM_MARGIN * 2);
    }
};

ThumbnailBarView::ThumbnailBarView(QWidget *parent)
    : ThumbnailView(parent)
    , d(new ThumbnailBarViewPrivate)
{
    d->q = this;
    d->mTimeLine = new QTimeLine(SMOOTH_SCROLL_DURATION, this);
    connect(d->mTimeLine, &QTimeLine::frameChanged, this, &ThumbnailBarView::slotFrameChanged);

    d->mRowCount = 1;
    d->mOrientation = Qt::Vertical; // To pass value-has-changed check in setOrientation()
    setOrientation(Qt::Horizontal);

    setObjectName(QStringLiteral("thumbnailBarView"));
    setWrapping(true);

#ifdef WINDOWS_PROXY_STYLE
    d->mStyle = new ProxyStyle;
    setStyle(d->mStyle);
#endif
}

ThumbnailBarView::~ThumbnailBarView()
{
#ifdef WINDOWS_PROXY_STYLE
    delete d->mStyle;
#endif
    delete d;
}

Qt::Orientation ThumbnailBarView::orientation() const
{
    return d->mOrientation;
}

void ThumbnailBarView::setOrientation(Qt::Orientation orientation)
{
    if (d->mOrientation == orientation) {
        return;
    }
    d->mOrientation = orientation;

    if (d->mOrientation == Qt::Vertical) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        setFlow(LeftToRight);
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFlow(TopToBottom);
    }

    d->updateMinMaxSizes();
}

void ThumbnailBarView::slotFrameChanged(int value)
{
    d->scrollBar()->setValue(value);
}

void ThumbnailBarView::resizeEvent(QResizeEvent *event)
{
    ThumbnailView::resizeEvent(event);
    d->updateThumbnailSize();
}

void ThumbnailBarView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    ThumbnailView::selectionChanged(selected, deselected);

    QModelIndexList oldList = deselected.indexes();
    QModelIndexList newList = selected.indexes();
    // Only scroll the list if the user went from one image to another. If the
    // user just unselected one image from a set of two, he might want to
    // reselect it again, scrolling the thumbnails would prevent him from
    // reselecting it by clicking again without moving the mouse.
    if (oldList.count() == 1 && newList.count() == 1 && isVisible()) {
        d->smoothScrollTo(newList.first());
    }
}

void ThumbnailBarView::wheelEvent(QWheelEvent *event)
{
    d->scrollBar()->setValue(d->scrollBar()->value() - event->angleDelta().y());
}

int ThumbnailBarView::rowCount() const
{
    return d->mRowCount;
}

void ThumbnailBarView::setRowCount(int rowCount)
{
    Q_ASSERT(rowCount > 0);
    d->mRowCount = rowCount;
    d->updateMinMaxSizes();
    d->updateThumbnailSize();
}

} // namespace
