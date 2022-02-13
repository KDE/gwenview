// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#include "hud/hudbutton.h"

// Local
#include "gwenview_lib_debug.h"
#include <hud/hudtheme.h>

// KF
#include <KIconLoader>

// Qt
#include <QAction>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGraphicsSceneEvent>
#include <QIcon>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

namespace Gwenview
{
struct LayoutInfo {
    QRect iconRect;
    QRect textRect;
    QSize size;
};

struct HudButtonPrivate {
    HudButton *q = nullptr;
    QAction *mAction = nullptr;

    QPalette mDarkPalette;
    QPixmap mLightIcon;
    KIconLoader::Group mIconGroup;
    QString mText;

    bool mIsDown;

    void initLayoutInfo(LayoutInfo *info, const QSizeF &constraint)
    {
        HudTheme::RenderInfo renderInfo = HudTheme::renderInfo(HudTheme::ButtonWidget);
        const int padding = renderInfo.padding;
        QSize minInnerSize = constraint.toSize() - QSize(2 * padding, 2 * padding);

        if (!mLightIcon.isNull()) {
            const int size = KIconLoader::global()->currentSize(mIconGroup);
            info->iconRect = QRect(padding, padding, size, qMax(size, minInnerSize.height()));
            minInnerSize.rwidth() -= size;
        }
        if (!mText.isEmpty()) {
            const QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
            QFontMetrics fm(font);
            const QSize size = fm.size(0, mText).expandedTo(minInnerSize);
            info->textRect = QRect(padding, padding, size.width(), size.height());
            if (!info->iconRect.isNull()) {
                info->textRect.translate(info->iconRect.right(), 0);
            }
        }

        const QRectF rect = info->iconRect | info->textRect;
        info->size = QSize(rect.right() + padding, rect.bottom() + padding);
    }

    void initFromAction()
    {
        Q_ASSERT(mAction);
        q->setIcon(mAction->icon());
        q->setText(mAction->text());
    }
};

HudButton::HudButton(QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , d(new HudButtonPrivate)
{
    d->q = this;
    d->mAction = nullptr;
    d->mIsDown = false;

    // Palette to use for generating light icon. All we need is a light foreground.
    d->mDarkPalette = palette();
    d->mDarkPalette.setColor(QPalette::WindowText, Qt::white);
    d->mIconGroup = KIconLoader::Small;

    setCursor(Qt::ArrowCursor);
    setAcceptHoverEvents(true);
}

HudButton::~HudButton()
{
    delete d;
}

void HudButton::setIcon(const QIcon &icon)
{
    // Since the HUD is always drawn with a dark theme, we need to make sure
    // the icon is light, in order to contrast. We then cache this light icon
    // to avoid setting a custom palette on KIconLoader repeatedly.
    KIconLoader::global()->setCustomPalette(d->mDarkPalette);
    d->mLightIcon = icon.pixmap(KIconLoader::global()->currentSize(d->mIconGroup));
    KIconLoader::global()->resetPalette();
    updateGeometry();
}

void HudButton::setText(const QString &text)
{
    d->mText = text;
    updateGeometry();
}

void HudButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    HudTheme::State state;
    if (option->state.testFlag(QStyle::State_MouseOver)) {
        state = d->mIsDown ? HudTheme::DownState : HudTheme::MouseOverState;
    } else {
        state = HudTheme::NormalState;
    }
    HudTheme::RenderInfo renderInfo = HudTheme::renderInfo(HudTheme::ButtonWidget, state);

    painter->setPen(renderInfo.borderPen);
    painter->setBrush(renderInfo.bgBrush);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawRoundedRect(boundingRect().adjusted(.5, .5, -.5, -.5), renderInfo.borderRadius, renderInfo.borderRadius);

    LayoutInfo info;
    d->initLayoutInfo(&info, size());

    if (!d->mLightIcon.isNull()) {
        painter->drawPixmap(info.iconRect.topLeft(), d->mLightIcon);
    }
    if (!d->mText.isEmpty()) {
        painter->setPen(renderInfo.textPen);
        painter->drawText(info.textRect, Qt::AlignCenter, d->mText);
    }
}

QSizeF HudButton::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    LayoutInfo info;
    d->initLayoutInfo(&info, constraint);
    if (which == Qt::MinimumSize || which == Qt::PreferredSize) {
        return info.size;
    } else {
        return constraint.expandedTo(info.size);
    }
}

void HudButton::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    d->mIsDown = true;
    update();
}

void HudButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    d->mIsDown = false;
    update();
    if (boundingRect().contains(event->pos())) {
        Q_EMIT clicked();
    }
}

void HudButton::setDefaultAction(QAction *action)
{
    if (action != d->mAction) {
        d->mAction = action;
        if (!actions().contains(action)) {
            addAction(action);
        }
        d->initFromAction();
        connect(this, SIGNAL(clicked()), d->mAction, SLOT(trigger()));
    }
}

bool HudButton::event(QEvent *event)
{
    if (event->type() == QEvent::ActionChanged) {
        d->initFromAction();
    }
    return QGraphicsWidget::event(event);
}

} // namespace
