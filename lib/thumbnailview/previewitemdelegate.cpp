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
// Self
#include "previewitemdelegate.h"
#include <config-gwenview.h>

// Qt
#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QHash>
#include <QHoverEvent>
#include <QPainter>
#include <QPainterPath>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QToolButton>
#include <QUrl>

// KF
#include <KDirModel>
#include <KIconLoader>

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <KRatingPainter>
#endif

// Local
#include "archiveutils.h"
#include "gwenview_lib_debug.h"
#include "itemeditor.h"
#include "paintutils.h"
#include "thumbnailview.h"
#include "timeutils.h"
#include "tooltipwidget.h"
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "../semanticinfo/semanticinfodirmodel.h"
#endif

// Define this to be able to fine tune the rendering of the selection
// background through a config file
//#define FINETUNE_SELECTION_BACKGROUND
#ifdef FINETUNE_SELECTION_BACKGROUND
#include <QDir>
#include <QSettings>
#endif

//#define DEBUG_DRAW_BORDER
//#define DEBUG_DRAW_CURRENT

namespace Gwenview
{
/**
 * Space between the item outer rect and the content, and between the
 * thumbnail and the caption
 */
const int ITEM_MARGIN_DELEGATE = 5;

/** How darker is the border line around selection */
const int SELECTION_BORDER_DARKNESS = 140;
const int FOCUS_BORDER_DARKNESS = 200;

/** Radius of the selection rounded corners, in pixels */
const int SELECTION_RADIUS = 5;

/** Space between the item outer rect and the context bar */
const int CONTEXTBAR_MARGIN = 1;

/** How dark is the shadow, 0 is invisible, 255 is as dark as possible */
const int SHADOW_STRENGTH_DELEGATE = 128;

/** How many pixels around the thumbnail are shadowed */
const int SHADOW_SIZE_DELEGATE = 4;

static KFileItem fileItemForIndexThumbnailView(const QModelIndex &index)
{
    Q_ASSERT(index.isValid());
    QVariant data = index.data(KDirModel::FileItemRole);
    return qvariant_cast<KFileItem>(data);
}

static QUrl urlForIndexThumbnailView(const QModelIndex &index)
{
    KFileItem item = fileItemForIndexThumbnailView(index);
    return item.url();
}

struct PreviewItemDelegatePrivate {
    /**
     * Maps full text to elided text.
     */
    mutable QHash<QString, QString> mElidedTextCache;

    // Key is height * 1000 + width
    using ShadowCache = QHash<int, QPixmap>;
    mutable ShadowCache mShadowCache;

    PreviewItemDelegate *q;
    QPointer<ThumbnailView> mView;
    QWidget *mContextBar;
    QToolButton *mSaveButton;
    QPixmap mSaveButtonPixmap;

    QToolButton *mToggleSelectionButton;
    QToolButton *mFullScreenButton;
    QToolButton *mRotateLeftButton;
    QToolButton *mRotateRightButton;
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    KRatingPainter mRatingPainter;
#endif

    QPersistentModelIndex mIndexUnderCursor;
    QSize mThumbnailSize;
    PreviewItemDelegate::ThumbnailDetails mDetails;
    PreviewItemDelegate::ContextBarActions mContextBarActions;
    Qt::TextElideMode mTextElideMode;

    QPointer<ToolTipWidget> mToolTip;
    QScopedPointer<QAbstractAnimation> mToolTipAnimation;

    void initSaveButtonPixmap()
    {
        if (!mSaveButtonPixmap.isNull()) {
            return;
        }
        // Necessary otherwise we won't see the save button itself
        mSaveButton->adjustSize();

        mSaveButtonPixmap = QPixmap(mSaveButton->sizeHint());
        mSaveButtonPixmap.fill(Qt::transparent);
        mSaveButton->render(&mSaveButtonPixmap, QPoint(), QRegion(), QWidget::DrawChildren);
    }

    void showContextBar(const QRect &rect, const QPixmap &thumbnailPix)
    {
        if (mContextBarActions == PreviewItemDelegate::NoAction) {
            return;
        }
        mContextBar->adjustSize();
        // Center bar, except if only showing SelectionAction.
        const int posX = mContextBarActions == PreviewItemDelegate::SelectionAction ? 0 : (rect.width() - mContextBar->width()) / 2;
        const int thumbnailPixHeight = qRound(thumbnailPix.height() / thumbnailPix.devicePixelRatio());
        const int posY = qMax(CONTEXTBAR_MARGIN, mThumbnailSize.height() - thumbnailPixHeight - mContextBar->height());
        mContextBar->move(rect.topLeft() + QPoint(posX, posY));
        mContextBar->show();
    }

    void initToolTip()
    {
        mToolTip = new ToolTipWidget(mView->viewport());
        mToolTip->setOpacity(0);
        mToolTip->show();
    }

    bool hoverEventFilter(QHoverEvent *event)
    {
        QModelIndex index = mView->indexAt(event->pos());
        if (index != mIndexUnderCursor) {
            updateHoverUi(index);
        } else {
            // Same index, nothing to do, but repaint anyway in case we are
            // over the rating row
            mView->update(mIndexUnderCursor);
        }
        return false;
    }

    void updateHoverUi(const QModelIndex &index)
    {
        QModelIndex oldIndex = mIndexUnderCursor;
        mIndexUnderCursor = index;
        mView->update(oldIndex);

        if (QApplication::style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, mView)) {
            mView->setCursor(mIndexUnderCursor.isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);
        }

        if (mIndexUnderCursor.isValid()) {
            updateToggleSelectionButton();
            updateImageButtons();

            const QRect rect = mView->visualRect(mIndexUnderCursor);
            const QPixmap thumbnailPix = mView->thumbnailForIndex(index);
            showContextBar(rect, thumbnailPix);
            if (mView->isModified(mIndexUnderCursor)) {
                showSaveButton(rect);
            } else {
                mSaveButton->hide();
            }

            showToolTip(index);
            mView->update(mIndexUnderCursor);

        } else {
            mContextBar->hide();
            mSaveButton->hide();
            hideToolTip();
        }
    }

    QRect ratingRectFromIndexRect(const QRect &rect) const
    {
        return QRect(rect.left(), rect.bottom() - ratingRowHeight() - ITEM_MARGIN_DELEGATE, rect.width(), ratingRowHeight());
    }

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    int ratingFromCursorPosition(const QRect &ratingRect) const
    {
        const QPoint pos = mView->viewport()->mapFromGlobal(QCursor::pos());
        return mRatingPainter.ratingFromPosition(ratingRect, pos);
    }
#endif

    bool mouseButtonEventFilter(QEvent::Type type)
    {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        const QRect rect = ratingRectFromIndexRect(mView->visualRect(mIndexUnderCursor));
        const int rating = ratingFromCursorPosition(rect);
        if (rating == -1) {
            return false;
        }
        if (type == QEvent::MouseButtonRelease) {
            q->setDocumentRatingRequested(urlForIndexThumbnailView(mIndexUnderCursor), rating);
        }
        return true;
#else
        return false;
#endif
    }

    QPoint saveButtonPosition(const QRect &itemRect) const
    {
        QSize buttonSize = mSaveButton->sizeHint();
        int posX = itemRect.right() - buttonSize.width();
        int posY = itemRect.top() + mThumbnailSize.height() + 2 * ITEM_MARGIN_DELEGATE - buttonSize.height();

        return QPoint(posX, posY);
    }

    void showSaveButton(const QRect &itemRect) const
    {
        mSaveButton->move(saveButtonPosition(itemRect));
        mSaveButton->show();
    }

    void drawBackground(QPainter *painter, const QRect &rect, const QColor &bgColor, const QColor &borderColor) const
    {
        int bgH, bgS, bgV;
        int borderH, borderS, borderV, borderMargin;
#ifdef FINETUNE_SELECTION_BACKGROUND
        QSettings settings(QDir::homePath() + "/colors.ini", QSettings::IniFormat);
        bgH = settings.value("bg/h").toInt();
        bgS = settings.value("bg/s").toInt();
        bgV = settings.value("bg/v").toInt();
        borderH = settings.value("border/h").toInt();
        borderS = settings.value("border/s").toInt();
        borderV = settings.value("border/v").toInt();
        borderMargin = settings.value("border/margin").toInt();
#else
        bgH = 0;
        bgS = -20;
        bgV = 43;
        borderH = 0;
        borderS = -100;
        borderV = 60;
        borderMargin = 1;
#endif
        painter->setRenderHint(QPainter::Antialiasing);

        QRectF rectF = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

        QPainterPath path = PaintUtils::roundedRectangle(rectF, SELECTION_RADIUS);

        QLinearGradient gradient(rectF.topLeft(), rectF.bottomLeft());
        gradient.setColorAt(0, PaintUtils::adjustedHsv(bgColor, bgH, bgS, bgV));
        gradient.setColorAt(1, bgColor);
        painter->fillPath(path, gradient);

        painter->setPen(borderColor);
        painter->drawPath(path);

        painter->setPen(PaintUtils::adjustedHsv(borderColor, borderH, borderS, borderV));
        rectF = rectF.adjusted(borderMargin, borderMargin, -borderMargin, -borderMargin);
        path = PaintUtils::roundedRectangle(rectF, SELECTION_RADIUS);
        painter->drawPath(path);
    }

    void drawShadow(QPainter *painter, const QRect &rect) const
    {
        const QPoint shadowOffset(-SHADOW_SIZE_DELEGATE, -SHADOW_SIZE_DELEGATE + 1);

        const auto dpr = painter->device()->devicePixelRatioF();
        int key = qRound((rect.height() * 1000 + rect.width()) * dpr);

        ShadowCache::Iterator it = mShadowCache.find(key);
        if (it == mShadowCache.end()) {
            QSize size = QSize(rect.width() + 2 * SHADOW_SIZE_DELEGATE, rect.height() + 2 * SHADOW_SIZE_DELEGATE);
            QColor color(0, 0, 0, SHADOW_STRENGTH_DELEGATE);
            QPixmap shadow = PaintUtils::generateFuzzyRect(size * dpr, color, qRound(SHADOW_SIZE_DELEGATE * dpr));
            shadow.setDevicePixelRatio(dpr);
            it = mShadowCache.insert(key, shadow);
        }
        painter->drawPixmap(rect.topLeft() + shadowOffset, it.value());
    }

    void drawText(QPainter *painter, const QRect &rect, const QColor &fgColor, const QString &fullText) const
    {
        QFontMetrics fm = mView->fontMetrics();

        // Elide text
        QString text;
        QHash<QString, QString>::const_iterator it = mElidedTextCache.constFind(fullText);
        if (it == mElidedTextCache.constEnd()) {
            text = fm.elidedText(fullText, mTextElideMode, rect.width());
            mElidedTextCache[fullText] = text;
        } else {
            text = it.value();
        }

        // Compute x pos
        int posX;
        if (text.length() == fullText.length()) {
            // Not elided, center text
            posX = (rect.width() - fm.boundingRect(text).width()) / 2;
        } else {
            // Elided, left align
            posX = 0;
        }

        // Draw text
        painter->setPen(fgColor);
        painter->drawText(rect.left() + posX, rect.top() + fm.ascent(), text);
    }

    void drawRating(QPainter *painter, const QRect &rect, const QVariant &value)
    {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        const int rating = value.toInt();
        const QRect ratingRect = ratingRectFromIndexRect(rect);
        const int hoverRating = ratingFromCursorPosition(ratingRect);
        mRatingPainter.paint(painter, ratingRect, rating, hoverRating);
#endif
    }

    bool isTextElided(const QString &text) const
    {
        QHash<QString, QString>::const_iterator it = mElidedTextCache.constFind(text);
        if (it == mElidedTextCache.constEnd()) {
            return false;
        }
        return it.value().length() < text.length();
    }

    /**
     * Show a tooltip only if the item has been elided.
     * This function places the tooltip over the item text.
     */
    void showToolTip(const QModelIndex &index)
    {
        if (mDetails == 0 || mDetails == PreviewItemDelegate::RatingDetail) {
            // No text to display
            return;
        }

        // Gather tip text
        QStringList textList;
        bool elided = false;
        if (mDetails & PreviewItemDelegate::FileNameDetail) {
            const QString text = index.data().toString();
            elided |= isTextElided(text);
            textList << text;
        }

        // FIXME: Duplicated from drawText
        const KFileItem fileItem = fileItemForIndexThumbnailView(index);
        const bool isDirOrArchive = ArchiveUtils::fileItemIsDirOrArchive(fileItem);
        if (mDetails & PreviewItemDelegate::DateDetail) {
            if (!ArchiveUtils::fileItemIsDirOrArchive(fileItem)) {
                const QDateTime dt = TimeUtils::dateTimeForFileItem(fileItem);
                const QString text = QLocale().toString(dt, QLocale::ShortFormat);
                elided |= isTextElided(text);
                textList << text;
            }
        }

        if (!isDirOrArchive && (mDetails & PreviewItemDelegate::ImageSizeDetail)) {
            QSize fullSize;
            QPixmap thumbnailPix = mView->thumbnailForIndex(index, &fullSize);
            if (fullSize.isValid()) {
                const QString text = QStringLiteral("%1x%2").arg(fullSize.width()).arg(fullSize.height());
                elided |= isTextElided(text);
                textList << text;
            }
        }

        if (!isDirOrArchive && (mDetails & PreviewItemDelegate::FileSizeDetail)) {
            const KIO::filesize_t size = fileItem.size();
            if (size > 0) {
                const QString text = KIO::convertSize(size);
                elided |= isTextElided(text);
                textList << text;
            }
        }

        if (!elided) {
            hideToolTip();
            return;
        }

        bool newTipLabel = !mToolTip;
        if (!mToolTip) {
            initToolTip();
        }
        mToolTip->setText(textList.join(QLatin1Char('\n')));
        QSize tipSize = mToolTip->sizeHint();

        // Compute tip position
        QRect rect = mView->visualRect(index);
        const int textY = ITEM_MARGIN_DELEGATE + mThumbnailSize.height() + ITEM_MARGIN_DELEGATE;
        const int spacing = 1;
        QRect geometry(QPoint(rect.topLeft() + QPoint((rect.width() - tipSize.width()) / 2, textY + spacing)), tipSize);
        if (geometry.left() < 0) {
            geometry.moveLeft(0);
        } else if (geometry.right() > mView->viewport()->width()) {
            geometry.moveRight(mView->viewport()->width());
        }

        // Show tip
        auto *anim = new QParallelAnimationGroup();
        auto *fadeIn = new QPropertyAnimation(mToolTip, "opacity");
        fadeIn->setStartValue(mToolTip->opacity());
        fadeIn->setEndValue(1.);
        anim->addAnimation(fadeIn);

        if (newTipLabel) {
            mToolTip->setGeometry(geometry);
        } else {
            auto *move = new QPropertyAnimation(mToolTip, "geometry");
            move->setStartValue(mToolTip->geometry());
            move->setEndValue(geometry);
            anim->addAnimation(move);
        }

        mToolTipAnimation.reset(anim);
        mToolTipAnimation->start();
    }

    void hideToolTip()
    {
        if (!mToolTip) {
            return;
        }
        auto *anim = new QSequentialAnimationGroup();
        if (mToolTipAnimation->state() == QPropertyAnimation::Stopped) {
            anim->addPause(500);
        }
        auto *fadeOut = new QPropertyAnimation(mToolTip, "opacity");
        fadeOut->setStartValue(mToolTip->opacity());
        fadeOut->setEndValue(0.);
        anim->addAnimation(fadeOut);
        mToolTipAnimation.reset(anim);
        mToolTipAnimation->start();
        QObject::connect(anim, &QSequentialAnimationGroup::finished, mToolTip.data(), &ToolTipWidget::deleteLater);
    }

    int itemWidth() const
    {
        return mThumbnailSize.width() + 2 * ITEM_MARGIN_DELEGATE;
    }

    int ratingRowHeight() const
    {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        return qMax(mView->fontMetrics().ascent(), int(KIconLoader::SizeSmall));
#else
        return 0;
#endif
    }

    int itemHeight() const
    {
        const int lineHeight = mView->fontMetrics().height();
        int textHeight = 0;
        if (mDetails & PreviewItemDelegate::FileNameDetail) {
            textHeight += lineHeight;
        }
        if (mDetails & PreviewItemDelegate::DateDetail) {
            textHeight += lineHeight;
        }
        if (mDetails & PreviewItemDelegate::ImageSizeDetail) {
            textHeight += lineHeight;
        }
        if (mDetails & PreviewItemDelegate::FileSizeDetail) {
            textHeight += lineHeight;
        }
        if (mDetails & PreviewItemDelegate::RatingDetail) {
            textHeight += ratingRowHeight();
        }
        if (textHeight == 0) {
            // Keep at least one row of text, so that we can show folder names
            textHeight = lineHeight;
        }
        return mThumbnailSize.height() + textHeight + 3 * ITEM_MARGIN_DELEGATE;
    }

    void selectIndexUnderCursorIfNoMultiSelection()
    {
        if (mView->selectionModel()->selectedIndexes().size() <= 1) {
            mView->setCurrentIndex(mIndexUnderCursor);
        }
    }

    void updateToggleSelectionButton()
    {
        mToggleSelectionButton->setIcon(
            QIcon::fromTheme(mView->selectionModel()->isSelected(mIndexUnderCursor) ? QStringLiteral("list-remove") : QStringLiteral("list-add")));
    }

    void updateImageButtons()
    {
        const KFileItem item = fileItemForIndexThumbnailView(mIndexUnderCursor);
        const bool isImage = !ArchiveUtils::fileItemIsDirOrArchive(item);
        mFullScreenButton->setEnabled(isImage);
        mRotateLeftButton->setEnabled(isImage);
        mRotateRightButton->setEnabled(isImage);
    }

    void updateContextBar()
    {
        if (mContextBarActions == PreviewItemDelegate::NoAction) {
            mContextBar->hide();
            return;
        }
        const int width = itemWidth();
        const int buttonWidth = mRotateRightButton->sizeHint().width();
        mFullScreenButton->setVisible(mContextBarActions & PreviewItemDelegate::FullScreenAction);
        bool rotate = mContextBarActions & PreviewItemDelegate::RotateAction;
        mRotateLeftButton->setVisible(rotate && width >= 3 * buttonWidth);
        mRotateRightButton->setVisible(rotate && width >= 4 * buttonWidth);
        mContextBar->adjustSize();
    }

    void updateViewGridSize()
    {
        mView->setGridSize(QSize(itemWidth(), itemHeight()));
    }
};

PreviewItemDelegate::PreviewItemDelegate(ThumbnailView *view)
    : QItemDelegate(view)
    , d(new PreviewItemDelegatePrivate)
{
    d->q = this;
    d->mView = view;
    view->viewport()->installEventFilter(this);

    // Set this attribute so that the viewport receives QEvent::HoverMove and
    // QEvent::HoverLeave events. We use these events in the event filter
    // installed on the viewport.
    // Some styles set this attribute themselves (Oxygen and Skulpture do) but
    // others do not (Plastique, Cleanlooks...)
    view->viewport()->setAttribute(Qt::WA_Hover);

    d->mThumbnailSize = view->thumbnailSize();
    d->mDetails = FileNameDetail;
    d->mContextBarActions = SelectionAction | FullScreenAction | RotateAction;
    d->mTextElideMode = Qt::ElideRight;

    connect(view, &ThumbnailView::rowsRemovedSignal, this, &PreviewItemDelegate::slotRowsChanged);
    connect(view, &ThumbnailView::rowsInsertedSignal, this, &PreviewItemDelegate::slotRowsChanged);
    connect(view, &ThumbnailView::selectionChangedSignal, [this]() {
        d->updateToggleSelectionButton();
    });

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    d->mRatingPainter.setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    d->mRatingPainter.setLayoutDirection(view->layoutDirection());
    d->mRatingPainter.setMaxRating(10);
#endif

    connect(view, &ThumbnailView::thumbnailSizeChanged, this, &PreviewItemDelegate::setThumbnailSize);

    // Button frame
    d->mContextBar = new QWidget(d->mView->viewport());
    d->mContextBar->hide();

    d->mToggleSelectionButton = new QToolButton;
    d->mToggleSelectionButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    connect(d->mToggleSelectionButton, &QToolButton::clicked, this, &PreviewItemDelegate::slotToggleSelectionClicked);

    d->mFullScreenButton = new QToolButton;
    d->mFullScreenButton->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
    connect(d->mFullScreenButton, &QToolButton::clicked, this, &PreviewItemDelegate::slotFullScreenClicked);

    d->mRotateLeftButton = new QToolButton;
    d->mRotateLeftButton->setIcon(QIcon::fromTheme(QStringLiteral("object-rotate-left")));
    connect(d->mRotateLeftButton, &QToolButton::clicked, this, &PreviewItemDelegate::slotRotateLeftClicked);

    d->mRotateRightButton = new QToolButton;
    d->mRotateRightButton->setIcon(QIcon::fromTheme(QStringLiteral("object-rotate-right")));
    connect(d->mRotateRightButton, &QToolButton::clicked, this, &PreviewItemDelegate::slotRotateRightClicked);

    auto *layout = new QHBoxLayout(d->mContextBar);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->addWidget(d->mToggleSelectionButton);
    layout->addWidget(d->mFullScreenButton);
    layout->addWidget(d->mRotateLeftButton);
    layout->addWidget(d->mRotateRightButton);

    // Save button
    d->mSaveButton = new QToolButton(d->mView->viewport());
    d->mSaveButton->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    d->mSaveButton->hide();
    connect(d->mSaveButton, &QToolButton::clicked, this, &PreviewItemDelegate::slotSaveClicked);
}

PreviewItemDelegate::~PreviewItemDelegate()
{
    delete d;
}

QSize PreviewItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    return d->mView->gridSize();
}

bool PreviewItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Destroy || event->type() == QEvent::ChildRemoved || !d->mView) {
        return QItemDelegate::eventFilter(object, event);
    }

    if (object == d->mView->viewport()) {
        switch (event->type()) {
        case QEvent::ToolTip:
            return true;

        case QEvent::HoverMove:
        case QEvent::HoverLeave:
            return d->hoverEventFilter(static_cast<QHoverEvent *>(event));

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            return d->mouseButtonEventFilter(event->type());

        default:
            return false;
        }
    } else {
        // Necessary for the item editor to work correctly (especially closing
        // the editor with the Escape key)
        return QItemDelegate::eventFilter(object, event);
    }
}

void PreviewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int thumbnailHeight = d->mThumbnailSize.height();
    QSize fullSize;
    QPixmap thumbnailPix = d->mView->thumbnailForIndex(index, &fullSize);
    QSize thumbnailSize = thumbnailPix.size() / thumbnailPix.devicePixelRatio();
    const KFileItem fileItem = fileItemForIndexThumbnailView(index);
    const bool opaque = !thumbnailPix.hasAlphaChannel();
    const bool isDirOrArchive = ArchiveUtils::fileItemIsDirOrArchive(fileItem);
    QRect rect = option.rect;
    const bool selected = option.state & QStyle::State_Selected;
    const bool underMouse = option.state & QStyle::State_MouseOver;
    const bool hasFocus = option.state & QStyle::State_HasFocus;

    const QWidget *viewport = d->mView->viewport();

#ifdef DEBUG_DRAW_BORDER
    painter->setPen(Qt::red);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect);
#endif

    // Select color group
    QPalette::ColorGroup cg;

    if ((option.state & QStyle::State_Enabled) && (option.state & QStyle::State_Active)) {
        cg = QPalette::Normal;
    } else if ((option.state & QStyle::State_Enabled)) {
        cg = QPalette::Inactive;
    } else {
        cg = QPalette::Disabled;
    }

    // Select colors
    QColor bgColor, borderColor, fgColor;

    fgColor = viewport->palette().color(viewport->foregroundRole());

    if (selected || underMouse) {
        bgColor = option.palette.color(cg, QPalette::Highlight);

        if (hasFocus) {
            borderColor = bgColor.darker(FOCUS_BORDER_DARKNESS);
        } else {
            borderColor = bgColor.darker(SELECTION_BORDER_DARKNESS);
        }
    } else {
        bgColor = viewport->palette().color(viewport->backgroundRole());
        if (hasFocus) {
            borderColor = fgColor;
        } else {
            borderColor = bgColor.lighter(200);
        }
    }

    // Compute thumbnailRect
    QRect thumbnailRect = QRect(rect.left() + (rect.width() - thumbnailSize.width()) / 2,
                                rect.top() + (thumbnailHeight - thumbnailSize.height()) + ITEM_MARGIN_DELEGATE,
                                thumbnailSize.width(),
                                thumbnailSize.height());

    // Draw background
    const QRect backgroundRect = thumbnailRect.adjusted(-ITEM_MARGIN_DELEGATE, -ITEM_MARGIN_DELEGATE, ITEM_MARGIN_DELEGATE, ITEM_MARGIN_DELEGATE);
    if (selected) {
        d->drawBackground(painter, backgroundRect, bgColor, borderColor);
    } else if (underMouse) {
        painter->setOpacity(0.2);
        d->drawBackground(painter, backgroundRect, bgColor, borderColor);
        painter->setOpacity(1.);
    } else if (opaque) {
        d->drawShadow(painter, thumbnailRect);
    }

    // Draw thumbnail
    if (opaque) {
        painter->setPen(borderColor);
        painter->setRenderHint(QPainter::Antialiasing, false);
        QRect borderRect = thumbnailRect.adjusted(-1, -1, 0, 0);
        painter->drawRect(borderRect);
    } else if (hasFocus && !selected) {
        painter->setPen(option.palette.color(cg, QPalette::Highlight));
        painter->setRenderHint(QPainter::Antialiasing, false);
        QLine underLine = QLine(thumbnailRect.bottomLeft(), thumbnailRect.bottomRight());
        underLine.translate(0, 3);
        painter->drawLine(underLine);
    }
    painter->drawPixmap(thumbnailRect.left(), thumbnailRect.top(), thumbnailPix);

    // Draw modified indicator
    bool isModified = d->mView->isModified(index);
    if (isModified) {
        // Draws a pixmap of the save button frame, as an indicator that
        // the image has been modified
        QPoint framePosition = d->saveButtonPosition(rect);
        d->initSaveButtonPixmap();
        painter->drawPixmap(framePosition, d->mSaveButtonPixmap);
    }

    // Draw busy indicator
    if (d->mView->isBusy(index)) {
        QPixmap pix = d->mView->busySequenceCurrentPixmap();
        painter->drawPixmap(thumbnailRect.left() + (thumbnailRect.width() - pix.width()) / 2,
                            thumbnailRect.top() + (thumbnailRect.height() - pix.height()) / 2,
                            pix);
    }

    if (index == d->mIndexUnderCursor) {
        // Show bar again: if the thumbnail has changed, we may need to update
        // its position. Don't do it if we are over rotate buttons, though: it
        // would not be nice to move the button now, the user may want to
        // rotate the image one more time.
        // The button will get moved when the mouse leaves.
        if (!d->mRotateLeftButton->underMouse() && !d->mRotateRightButton->underMouse()) {
            d->showContextBar(rect, thumbnailPix);
        }
        if (isModified) {
            // If we just rotated the image with the buttons from the
            // button frame, we need to show the save button frame right now.
            d->showSaveButton(rect);
        } else {
            d->mSaveButton->hide();
        }
    }

    QRect textRect(rect.left() + ITEM_MARGIN_DELEGATE,
                   rect.top() + 2 * ITEM_MARGIN_DELEGATE + thumbnailHeight,
                   rect.width() - 2 * ITEM_MARGIN_DELEGATE,
                   d->mView->fontMetrics().height());
    if (isDirOrArchive || (d->mDetails & PreviewItemDelegate::FileNameDetail)) {
        d->drawText(painter, textRect, fgColor, index.data().toString());
        textRect.moveTop(textRect.bottom());
    }

    if (!isDirOrArchive && (d->mDetails & PreviewItemDelegate::DateDetail)) {
        const QDateTime dt = TimeUtils::dateTimeForFileItem(fileItem);
        d->drawText(painter, textRect, fgColor, QLocale().toString(dt, QLocale::ShortFormat));
        textRect.moveTop(textRect.bottom());
    }

    if (!isDirOrArchive && (d->mDetails & PreviewItemDelegate::ImageSizeDetail)) {
        if (fullSize.isValid()) {
            const QString text = QStringLiteral("%1x%2").arg(fullSize.width()).arg(fullSize.height());
            d->drawText(painter, textRect, fgColor, text);
            textRect.moveTop(textRect.bottom());
        }
    }

    if (!isDirOrArchive && (d->mDetails & PreviewItemDelegate::FileSizeDetail)) {
        const KIO::filesize_t size = fileItem.size();
        if (size > 0) {
            const QString st = KIO::convertSize(size);
            d->drawText(painter, textRect, fgColor, st);
            textRect.moveTop(textRect.bottom());
        }
    }

    if (!isDirOrArchive && (d->mDetails & PreviewItemDelegate::RatingDetail)) {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        d->drawRating(painter, rect, index.data(SemanticInfoDirModel::RatingRole));
#endif
    }

#ifdef DEBUG_DRAW_CURRENT
    if (d->mView->currentIndex() == index) {
        painter->fillRect(rect.left(), rect.top(), 12, 12, Qt::red);
    }
#endif
}

void PreviewItemDelegate::setThumbnailSize(const QSize &value)
{
    d->mThumbnailSize = value;
    d->updateViewGridSize();
    d->updateContextBar();
    d->mElidedTextCache.clear();
}

void PreviewItemDelegate::slotSaveClicked()
{
    emit saveDocumentRequested(urlForIndexThumbnailView(d->mIndexUnderCursor));
}

void PreviewItemDelegate::slotRotateLeftClicked()
{
    d->selectIndexUnderCursorIfNoMultiSelection();
    emit rotateDocumentLeftRequested(urlForIndexThumbnailView(d->mIndexUnderCursor));
}

void PreviewItemDelegate::slotRotateRightClicked()
{
    d->selectIndexUnderCursorIfNoMultiSelection();
    emit rotateDocumentRightRequested(urlForIndexThumbnailView(d->mIndexUnderCursor));
}

void PreviewItemDelegate::slotFullScreenClicked()
{
    emit showDocumentInFullScreenRequested(urlForIndexThumbnailView(d->mIndexUnderCursor));
}

void PreviewItemDelegate::slotToggleSelectionClicked()
{
    d->mView->selectionModel()->select(d->mIndexUnderCursor, QItemSelectionModel::Toggle);
}

PreviewItemDelegate::ThumbnailDetails PreviewItemDelegate::thumbnailDetails() const
{
    return d->mDetails;
}

void PreviewItemDelegate::setThumbnailDetails(PreviewItemDelegate::ThumbnailDetails details)
{
    d->mDetails = details;
    d->updateViewGridSize();
    d->mView->scheduleDelayedItemsLayout();
}

PreviewItemDelegate::ContextBarActions PreviewItemDelegate::contextBarActions() const
{
    return d->mContextBarActions;
}

void PreviewItemDelegate::setContextBarActions(PreviewItemDelegate::ContextBarActions actions)
{
    d->mContextBarActions = actions;
    d->updateContextBar();
}

Qt::TextElideMode PreviewItemDelegate::textElideMode() const
{
    return d->mTextElideMode;
}

void PreviewItemDelegate::setTextElideMode(Qt::TextElideMode mode)
{
    if (d->mTextElideMode == mode) {
        return;
    }
    d->mTextElideMode = mode;
    d->mElidedTextCache.clear();
    d->mView->viewport()->update();
}

void PreviewItemDelegate::slotRowsChanged()
{
    // We need to update hover ui because the current index may have
    // disappeared: for example if the current image is removed with "del".
    QPoint pos = d->mView->viewport()->mapFromGlobal(QCursor::pos());
    QModelIndex index = d->mView->indexAt(pos);
    d->updateHoverUi(index);
}

QWidget *PreviewItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const
{
    return new ItemEditor(parent);
}

void PreviewItemDelegate::setEditorData(QWidget *widget, const QModelIndex &index) const
{
    auto *edit = qobject_cast<ItemEditor *>(widget);
    if (!edit) {
        return;
    }
    edit->setText(index.data().toString());
}

void PreviewItemDelegate::updateEditorGeometry(QWidget *widget, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto *edit = qobject_cast<ItemEditor *>(widget);
    if (!edit) {
        return;
    }
    QString text = index.data().toString();
    int textWidth = edit->fontMetrics().boundingRect(QLatin1String("  ") + text + QLatin1String("  ")).width();
    QRect textRect(option.rect.left() + (option.rect.width() - textWidth) / 2,
                   option.rect.top() + 2 * ITEM_MARGIN_DELEGATE + d->mThumbnailSize.height(),
                   textWidth,
                   edit->sizeHint().height());

    edit->setGeometry(textRect);
}

void PreviewItemDelegate::setModelData(QWidget *widget, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto *edit = qobject_cast<ItemEditor *>(widget);
    if (!edit) {
        return;
    }
    if (index.data().toString() != edit->text()) {
        model->setData(index, edit->text(), Qt::EditRole);
    }
}

} // namespace
