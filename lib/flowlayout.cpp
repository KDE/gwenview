/****************************************************************************
**
** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the example classes of the Qt Toolkit.
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License versions 2.0 or 3.0 as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file.  Please review the following information
** to ensure GNU General Public Licensing requirements will be met:
** https://www.fsf.org/licensing/licenses/info/GPLv2.html and
** https://www.gnu.org/copyleft/gpl.html.  In addition, as a special
** exception, Nokia gives you certain additional rights. These rights
** are described in the Nokia Qt GPL Exception version 1.3, included in
** the file GPL_EXCEPTION.txt in this package.
**
** Qt for Windows(R) Licensees
** As a special exception, Nokia, as the sole copyright holder for Qt
** Designer, grants users of the Qt/Eclipse Integration plug-in the
** right for the Qt/Eclipse Integration to link to functionality
** provided by Qt Designer and its related libraries.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

// Self
#include "flowlayout.h"

// Qt
#include <QHash>

FlowLayout::FlowLayout(QWidget *parent, int margin, int spacing)
    : QLayout(parent)
{
    setContentsMargins(margin, margin, margin, margin);
    setHorizontalSpacing(spacing);
    setVerticalSpacing(spacing);
}

FlowLayout::FlowLayout(int spacing)
{
    setHorizontalSpacing(spacing);
    setVerticalSpacing(spacing);
}

FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

int FlowLayout::horizontalSpacing() const
{
    return mHorizontalSpacing;
}

void FlowLayout::setHorizontalSpacing(const int spacing)
{
    mHorizontalSpacing = spacing;
}

int FlowLayout::verticalSpacing() const
{
    return mVerticalSpacing;
}

void FlowLayout::setVerticalSpacing(const int spacing)
{
    mVerticalSpacing = spacing;
}

void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

int FlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (QLayoutItem *item : std::as_const(itemList))
        size = size.expandedTo(item->minimumSize());

    size += QSize(2 * margin(), 2 * margin());
    return size;
}

void FlowLayout::addSpacing(const int size)
{
    addItem(new QSpacerItem(size, 0, QSizePolicy::Fixed, QSizePolicy::Minimum));
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    const int left = rect.x() + margin();
    int x = left;
    int y = rect.y() + margin();
    int lineHeight = 0;
    bool lastItemIsSpacer = false;
    QHash<int, int> widthForY;

    for (QLayoutItem *item : std::as_const(itemList)) {
        const bool itemIsSpacer = item->spacerItem() != nullptr;
        // Don't add invisible items or succeeding spacer items
        if (item->sizeHint().width() == 0 || (itemIsSpacer && lastItemIsSpacer)) {
            continue;
        }

        int nextX = x + item->sizeHint().width() + horizontalSpacing();
        if (nextX - horizontalSpacing() > rect.right() - margin() && lineHeight > 0) {
            x = left;
            y = y + lineHeight + verticalSpacing();
            nextX = x + item->sizeHint().width() + horizontalSpacing();
            lineHeight = 0;
        }

        // Don't place spacer items at start of line
        if (itemIsSpacer && x == left) {
            continue;
        }

        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

        x = nextX;
        // Don't add spacer items at end of line
        if (!itemIsSpacer) {
            widthForY[y] = x - margin();
        }
        lineHeight = qMax(lineHeight, item->sizeHint().height());
        lastItemIsSpacer = itemIsSpacer;
    }

    if (!testOnly) {
        const int contentWidth = rect.width() - 2 * margin();
        for (auto item : itemList) {
            QRect itemRect = item->geometry();
            // Center lines horizontally if flag AlignHCenter is set
            if (alignment() & Qt::AlignHCenter) {
                if (widthForY.contains(itemRect.y())) {
                    const int offset = (contentWidth - widthForY[itemRect.y()]) / 2;
                    itemRect.translate(offset, 0);
                }
            }
            // Center items vertically if flag AlignVCenter is set
            if (alignment() & Qt::AlignVCenter) {
                const int offset = (lineHeight - itemRect.height()) / 2;
                itemRect.translate(0, offset);
            }
            item->setGeometry(itemRect);
        }
    }

    return y + lineHeight - rect.y() + margin();
}
