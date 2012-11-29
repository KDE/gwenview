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
#include "graphicswidgetfloater.moc"

// Qt
#include <QEvent>
#include <QGraphicsWidget>
#include <QPointer>

// KDE
#include <KDebug>
#include <KDialog>

// Local

namespace Gwenview
{

struct GraphicsWidgetFloaterPrivate
{
    QGraphicsWidget* mParent;
    QPointer<QGraphicsWidget> mChild;
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

        childWidth = mChild->size().width();
        childHeight = mChild->size().height();

        parentWidth = mParent->size().width();
        parentHeight = mParent->size().height();

        if (parentWidth == 0 || parentHeight == 0) {
            return;
        }

        if (mAlignment & Qt::AlignLeft) {
            posX = mHorizontalMargin;
        } else if (mAlignment & Qt::AlignHCenter) {
            posX = (parentWidth - childWidth) / 2;
        } else if (mAlignment & Qt::AlignJustify) {
            posX = mHorizontalMargin;
            childWidth = parentWidth - 2 * mHorizontalMargin;
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

        mChild->setGeometry(posX, posY, childWidth, childHeight);

        mInsideUpdateChildGeometry = false;
    }
};

GraphicsWidgetFloater::GraphicsWidgetFloater(QGraphicsWidget* parent)
: QObject(parent)
, d(new GraphicsWidgetFloaterPrivate)
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

GraphicsWidgetFloater::~GraphicsWidgetFloater()
{
    delete d;
}

void GraphicsWidgetFloater::setChildWidget(QGraphicsWidget* child)
{
    if (d->mChild) {
        d->mChild->removeEventFilter(this);
        disconnect(d->mChild, 0, this, 0);
    }
    d->mChild = child;
    d->mChild->setParent(d->mParent);
    d->mChild->installEventFilter(this);
    connect(d->mChild, SIGNAL(visibleChanged()), SLOT(slotChildVisibilityChanged()));
    d->updateChildGeometry();
    //d->mChild->raise();
    d->mChild->show();
}

void GraphicsWidgetFloater::setAlignment(Qt::Alignment alignment)
{
    d->mAlignment = alignment;
    d->updateChildGeometry();
}

bool GraphicsWidgetFloater::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::GraphicsSceneResize) {
        d->updateChildGeometry();
    }
    return false;
}

void GraphicsWidgetFloater::slotChildVisibilityChanged()
{
    if (d->mChild->isVisible()) {
        d->updateChildGeometry();
    }
}

void GraphicsWidgetFloater::setHorizontalMargin(int value)
{
    d->mHorizontalMargin = value;
    d->updateChildGeometry();
}

int GraphicsWidgetFloater::horizontalMargin() const
{
    return d->mHorizontalMargin;
}

void GraphicsWidgetFloater::setVerticalMargin(int value)
{
    d->mVerticalMargin = value;
    d->updateChildGeometry();
}

int GraphicsWidgetFloater::verticalMargin() const
{
    return d->mVerticalMargin;
}

} // namespace
