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
#include <QStyleOptionToolButton>
#include <QStylePainter>

// KDE
#include <KLocalizedString>

namespace Gwenview
{

StatusBarToolButton::StatusBarToolButton(QWidget* parent)
: QToolButton(parent)
, mGroupPosition(NotGrouped)
{
    setToolButtonStyle(Qt::ToolButtonTextOnly);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

void StatusBarToolButton::setGroupPosition(StatusBarToolButton::GroupPosition groupPosition)
{
    mGroupPosition = groupPosition;
}

void StatusBarToolButton::paintEvent(QPaintEvent* event)
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
    QRect& panelRect = panelOpt.rect;
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
    const int y1 = opt.rect.top() + 6;
    const int y2 = opt.rect.bottom() - 6;
    if (mGroupPosition & GroupRight) {
        const int x = opt.rect.left();
        painter.setPen(opt.palette.color(QPalette::Light));
        painter.drawLine(x, y1, x, y2);
    }
    if (mGroupPosition & GroupLeft) {
        const int x = opt.rect.right();
        painter.setPen(opt.palette.color(QPalette::Mid));
        painter.drawLine(x, y1, x, y2);
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
        QAction* action = actions().constFirst();
        setToolTip(i18nc("@info:tooltip of custom toolbar button", "%1", action->toolTip()));
    }
}

} // namespace
