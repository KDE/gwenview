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
#include "widgetfloater.h"

// Qt
#include <QEvent>
#include <QPointer>
#include <QWidget>

// KDE
#include <KDebug>
#include <KDialog>

// Local

namespace Gwenview
{

struct WidgetFloaterPrivate
{
    QWidget* mParent;
    QPointer<QWidget> mChild;
    Qt::Alignment mAlignment;

    int mHorizontalMargin;
    int mVerticalMargin;
    bool mInsideUpdateChildGeometry;

    void updateChildGeometry()
    {
        if (!mChild) {
            return;
        }
        if (mInsideUpdateChildGeometry) {
            return;
        }
        mInsideUpdateChildGeometry = true;

        int posX, posY;
        int childWidth, childHeight;
        int parentWidth, parentHeight;

        childWidth = mChild->width();
        childHeight = mChild->height();

        parentWidth = mParent->width();
        parentHeight = mParent->height();

        if (mAlignment & Qt::AlignLeft) {
            posX = mHorizontalMargin;
        } else if (mAlignment & Qt::AlignHCenter) {
            posX = (parentWidth - childWidth) / 2;
        } else if (mAlignment & Qt::AlignJustify) {
            posX = mHorizontalMargin;
            childWidth = parentWidth - 2 * mHorizontalMargin;
            QRect childGeometry = mChild->geometry();
            childGeometry.setWidth(childWidth);
            mChild->setGeometry(childGeometry);
        } else {
            posX = parentWidth - childWidth - mHorizontalMargin;
        }

        if (mAlignment & Qt::AlignTop) {
            posY = mVerticalMargin;
        } else if (mAlignment & Qt::AlignVCenter) {
            posY = (parentHeight - childHeight) / 2;
        } else {
            posY = parentHeight - childHeight - mVerticalMargin;
        }

        mChild->move(posX, posY);

        mInsideUpdateChildGeometry = false;
    }
};

WidgetFloater::WidgetFloater(QWidget* parent)
: QObject(parent)
, d(new WidgetFloaterPrivate)
{
    Q_ASSERT(parent);
    d->mParent = parent;
    d->mParent->installEventFilter(this);
    d->mChild = 0;
    d->mAlignment = Qt::AlignCenter;
    d->mHorizontalMargin = KDialog::marginHint();
    d->mVerticalMargin = KDialog::marginHint();
    d->mInsideUpdateChildGeometry = false;
}

WidgetFloater::~WidgetFloater()
{
    delete d;
}

void WidgetFloater::setChildWidget(QWidget* child)
{
    if (d->mChild) {
        d->mChild->removeEventFilter(this);
    }
    d->mChild = child;
    d->mChild->setParent(d->mParent);
    d->mChild->installEventFilter(this);
    d->updateChildGeometry();
    d->mChild->raise();
    d->mChild->show();
}

void WidgetFloater::setAlignment(Qt::Alignment alignment)
{
    d->mAlignment = alignment;
    d->updateChildGeometry();
}

bool WidgetFloater::eventFilter(QObject*, QEvent* event)
{
    switch (event->type()) {
    case QEvent::Resize:
    case QEvent::Show:
        d->updateChildGeometry();
        break;
    default:
        break;
    }
    return false;
}

void WidgetFloater::setHorizontalMargin(int value)
{
    d->mHorizontalMargin = value;
    d->updateChildGeometry();
}

int WidgetFloater::horizontalMargin() const
{
    return d->mHorizontalMargin;
}

void WidgetFloater::setVerticalMargin(int value)
{
    d->mVerticalMargin = value;
    d->updateChildGeometry();
}

int WidgetFloater::verticalMargin() const
{
    return d->mVerticalMargin;
}

} // namespace
