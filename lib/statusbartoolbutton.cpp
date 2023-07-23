// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include <statusbartoolbutton.h>

// Qt
#include <QAction>
#include <QStyleOptionToolBar>
#include <QStyleOptionToolButton>
#include <QStylePainter>

// KF
#include <KLocalizedString>

namespace Gwenview
{
StatusBarToolButton::StatusBarToolButton(QWidget *parent)
    : QToolButton(parent)
    , mGroupPosition(NotGrouped)
{
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

QSize StatusBarToolButton::sizeHint() const
{
    QSize sizeHint = QToolButton::sizeHint();

    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    const bool isIconOnly = opt.toolButtonStyle == Qt::ToolButtonIconOnly || (!opt.icon.isNull() && opt.text.isEmpty());
    const bool isTextOnly = opt.toolButtonStyle == Qt::ToolButtonTextOnly || (opt.icon.isNull() && !opt.text.isEmpty());

    if (isIconOnly || isTextOnly) {
        QSize contentSize;
        if (isIconOnly) {
            contentSize.setHeight(qMax(opt.iconSize.height(), opt.fontMetrics.height()));
            contentSize.setWidth(contentSize.height());
        } else if (isTextOnly) {
            // copying the default text size behavior for text only QToolButtons
            QSize textSize = opt.fontMetrics.size(Qt::TextShowMnemonic, opt.text);
            // NOTE: QToolButton really does use horizontalAdvance() instead of boundingRect().width()
            textSize.setWidth(textSize.width() + opt.fontMetrics.horizontalAdvance(QLatin1Char(' ')) * 2);
            contentSize.setHeight(qMax(opt.iconSize.height(), textSize.height()));
            contentSize.setWidth(textSize.width());
        }
        if (popupMode() == MenuButtonPopup) {
            contentSize.setWidth(contentSize.width() + style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this));
        }
        sizeHint = style()->sizeFromContents(QStyle::CT_ToolButton, &opt, contentSize, this);
    }

    return sizeHint;
}

void StatusBarToolButton::setGroupPosition(StatusBarToolButton::GroupPosition groupPosition)
{
    mGroupPosition = groupPosition;
}

void StatusBarToolButton::paintEvent(QPaintEvent *event)
{
    if (mGroupPosition == NotGrouped) {
        QToolButton::paintEvent(event);
        return;
    }
    QStylePainter painter(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    QStyleOptionToolButton panelOpt = opt;

    // Panel
    QRect &panelRect = panelOpt.rect;
    switch (mGroupPosition) {
    case GroupLeft:
        panelRect.setWidth(panelRect.width() * 2);
        break;
    case GroupCenter:
        panelRect.setLeft(panelRect.left() - panelRect.width());
        panelRect.setWidth(panelRect.width() * 3);
        break;
    case GroupRight:
        panelRect.setLeft(panelRect.left() - panelRect.width());
        break;
    case NotGrouped:
        Q_ASSERT(0);
    }
    painter.drawPrimitive(QStyle::PE_PanelButtonTool, panelOpt);

    // Separator
    QStyleOptionToolBar tbOpt;
    tbOpt.palette = opt.palette;
    tbOpt.state = QStyle::State_Horizontal;
    const int width = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent);
    const int height = opt.rect.height() - 2;
    const int y = opt.rect.y() + (opt.rect.height() - height) / 2;
    if (mGroupPosition & GroupRight) {
        // Kinda hacky. This is needed because of the way toolbar separators are horizontally aligned.
        // Tested with Breeze (width: 8), Fusion (width: 6) and Oxygen (width: 8)
        const int x = opt.rect.left() - width / 1.5;
        tbOpt.rect = QRect(x, y, width, height);
        painter.drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, tbOpt);
    }
    if (mGroupPosition & GroupLeft) {
        const int x = opt.rect.right() - width / 2;
        tbOpt.rect = QRect(x, y, width, height);
        painter.drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, tbOpt);
    }

    // Text
    painter.drawControl(QStyle::CE_ToolButtonLabel, opt);

    // Filtering message on tooltip text for CJK to remove accelerators.
    // Quoting ktoolbar.cpp:
    // """
    // CJK languages use more verbose accelerator marker: they add a Latin
    // letter in parenthesis, and put accelerator on that. Hence, the default
    // removal of ampersand only may not be enough there, instead the whole
    // parenthesis construct should be removed. Provide these filtering i18n
    // messages so that translators can use Transcript for custom removal.
    // """
    if (!actions().isEmpty()) {
        QAction *action = actions().constFirst();
        setToolTip(i18nc("@info:tooltip of custom toolbar button", "%1", action->toolTip()));
    }
}

} // namespace

#include "moc_statusbartoolbutton.cpp"
