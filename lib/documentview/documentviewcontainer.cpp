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
#include "documentviewcontainer.moc"

// Local
#include <lib/documentview/documentview.h>
#include <lib/gwenviewconfig.h>

// KDE
#include <kdebug.h>
#include <kurl.h>

// Qt
#include <QEvent>
#include <QGLWidget>
#include <QGraphicsScene>
#include <QTimer>

// libc
#include <qmath.h>

namespace Gwenview
{

typedef QSet<DocumentView*> DocumentViewSet;

struct DocumentViewContainerPrivate {
    DocumentViewContainer* q;
    QGraphicsScene* mScene;
    DocumentViewSet mViews;
    DocumentViewSet mAddedViews;
    DocumentViewSet mRemovedViews;
    QTimer* mLayoutUpdateTimer;

    void scheduleLayoutUpdate()
    {
        mLayoutUpdateTimer->start();
    }

    /**
     * Remove view from set, move it to mRemovedViews so that it is later
     * deleted.
     */
    bool removeFromSet(DocumentView* view, DocumentViewSet* set)
    {
        DocumentViewSet::Iterator it = set->find(view);
        if (it == set->end()) {
            return false;
        }
        set->erase(it);
        mRemovedViews << view;
        scheduleLayoutUpdate();
        return true;
    }

    void resetSet(DocumentViewSet* set)
    {
        qDeleteAll(*set);
        set->clear();
    }
};

DocumentViewContainer::DocumentViewContainer(QWidget* parent)
: QGraphicsView(parent)
, d(new DocumentViewContainerPrivate)
{
    d->q = this;
    d->mScene = new QGraphicsScene(this);
    if (GwenviewConfig::animationMethod() == DocumentView::GLAnimation) {
        setViewport(new QGLWidget);
    }
    setScene(d->mScene);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    d->mLayoutUpdateTimer = new QTimer(this);
    d->mLayoutUpdateTimer->setInterval(0);
    d->mLayoutUpdateTimer->setSingleShot(true);
    connect(d->mLayoutUpdateTimer, SIGNAL(timeout()), SLOT(updateLayout()));
}

DocumentViewContainer::~DocumentViewContainer()
{
    delete d;
}

DocumentView* DocumentViewContainer::createView()
{
    DocumentView* view = new DocumentView(d->mScene);
    d->mAddedViews << view;
    view->show();
    connect(view, SIGNAL(animationFinished(DocumentView*)),
            SLOT(slotViewAnimationFinished(DocumentView*)));
    d->scheduleLayoutUpdate();
    return view;
}

void DocumentViewContainer::deleteView(DocumentView* view)
{
    if (d->removeFromSet(view, &d->mViews)) {
        return;
    }
    d->removeFromSet(view, &d->mAddedViews);
}

void DocumentViewContainer::reset()
{
    d->resetSet(&d->mViews);
    d->resetSet(&d->mAddedViews);
    d->resetSet(&d->mRemovedViews);
}

void DocumentViewContainer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateLayout();
}

void DocumentViewContainer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    d->mScene->setSceneRect(rect());
    updateLayout();
}

void DocumentViewContainer::updateLayout()
{
    // Stop update timer: this is useful if updateLayout() is called directly
    // and not through scheduleLayoutUpdate()
    d->mLayoutUpdateTimer->stop();
    DocumentViewSet views = d->mViews | d->mAddedViews;

    bool animated = GwenviewConfig::animationMethod() != DocumentView::NoAnimation;

    if (!views.isEmpty()) {
        // Compute column count
        int colCount;
        switch (views.count()) {
        case 1:
            colCount = 1;
            break;
        case 2:
            colCount = 2;
            break;
        case 3:
            colCount = 3;
            break;
        case 4:
            colCount = 2;
            break;
        case 5:
            colCount = 3;
            break;
        case 6:
            colCount = 3;
            break;
        default:
            colCount = 3;
            break;
        }

        int rowCount = qCeil(views.count() / qreal(colCount));
        Q_ASSERT(rowCount > 0);
        int viewWidth = width() / colCount;
        int viewHeight = height() / rowCount;

        int col = 0;
        int row = 0;

        Q_FOREACH(DocumentView * view, views) {
            QRect rect;
            rect.setLeft(col * viewWidth);
            rect.setTop(row * viewHeight);
            rect.setWidth(viewWidth);
            rect.setHeight(viewHeight);

            if (animated) {
                if (d->mViews.contains(view)) {
                    if (rect != view->geometry()) {
                        if (d->mAddedViews.isEmpty() && d->mRemovedViews.isEmpty()) {
                            // View moves because of a resize
                            view->moveTo(rect);
                        } else {
                            // View moves because the number of views changed,
                            // animate the change
                            view->moveToAnimated(rect);
                        }
                    }
                } else {
                    view->setGeometry(rect);
                    view->fadeIn();
                }
            } else {
                // Not animated, set final geometry and opacity now
                view->setGeometry(rect);
                view->setOpacity(1);
            }

            ++col;
            if (col == colCount) {
                col = 0;
                ++row;
            }
        }
    }

    if (animated) {
        Q_FOREACH(DocumentView* view, d->mRemovedViews) {
            view->fadeOut();
        }
    } else {
        QMetaObject::invokeMethod(this, "fakeViewAnimationsFinished", Qt::QueuedConnection);
    }
}

void DocumentViewContainer::fakeViewAnimationsFinished()
{
    // Animations are disabled. Pretend they are all done so that removed views
    // are deleted and added views are moved to mViews
    Q_FOREACH(DocumentView* view, d->mRemovedViews) {
        slotViewAnimationFinished(view);
    }
    Q_FOREACH(DocumentView* view, d->mAddedViews) {
        slotViewAnimationFinished(view);
    }
}

void DocumentViewContainer::slotViewAnimationFinished(DocumentView* view)
{
    if (d->mRemovedViews.contains(view)) {
        d->mRemovedViews.remove(view);
        delete view;
        return;
    }
    if (d->mAddedViews.contains(view)) {
        d->mAddedViews.remove(view);
        d->mViews.insert(view);
    }
}

} // namespace
