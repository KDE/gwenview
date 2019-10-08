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
#include "documentviewcontainer.h"

// Local
#include <lib/documentview/documentview.h>
#include <lib/graphicswidgetfloater.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>

// KDE

// Qt
#include <QGLWidget>
#include <QGraphicsScene>
#include <QPropertyAnimation>
#include <QTimer>
#include <QDebug>
#include <QtMath>

namespace Gwenview
{

typedef QSet<DocumentView*> DocumentViewSet;
typedef QHash<QUrl, DocumentView::Setup> SetupForUrl;

struct DocumentViewContainerPrivate
{
    DocumentViewContainer* q;
    QGraphicsScene* mScene;
    SetupForUrl mSetupForUrl;
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
        QGLWidget* glWidget = new QGLWidget;
        if (glWidget->isValid()) {
            setViewport(glWidget);
        } else {
            qWarning() << "Failed to initialize OpenGL support!";
            delete glWidget;
        }
    }
    setScene(d->mScene);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    d->mLayoutUpdateTimer = new QTimer(this);
    d->mLayoutUpdateTimer->setInterval(0);
    d->mLayoutUpdateTimer->setSingleShot(true);
    connect(d->mLayoutUpdateTimer, &QTimer::timeout, this, &DocumentViewContainer::updateLayout);

    connect(GwenviewConfig::self(), &GwenviewConfig::configChanged, this, &DocumentViewContainer::slotConfigChanged);
}

DocumentViewContainer::~DocumentViewContainer()
{
    delete d;
}

DocumentView* DocumentViewContainer::createView()
{
    DocumentView* view = new DocumentView(d->mScene);
    view->setPalette(palette());
    d->mAddedViews << view;
    view->show();
    connect(view, &DocumentView::fadeInFinished, this, &DocumentViewContainer::slotFadeInFinished);
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

DocumentView::Setup DocumentViewContainer::savedSetup(const QUrl& url) const
{
    return d->mSetupForUrl.value(url);
}

void DocumentViewContainer::updateSetup(DocumentView* view)
{
    d->mSetupForUrl[view->url()] = view->setup();
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

static bool viewLessThan(DocumentView* v1, DocumentView* v2)
{
    return v1->sortKey() < v2->sortKey();
}

void DocumentViewContainer::updateLayout()
{
    // Stop update timer: this is useful if updateLayout() is called directly
    // and not through scheduleLayoutUpdate()
    d->mLayoutUpdateTimer->stop();
    QList<DocumentView*> views = (d->mViews | d->mAddedViews).toList();
    std::sort(views.begin(), views.end(), viewLessThan);

    bool animated = GwenviewConfig::animationMethod() != DocumentView::NoAnimation;
    bool crossFade = d->mAddedViews.count() == 1 && d->mRemovedViews.count() == 1;

    if (animated && crossFade) {
        DocumentView* oldView = *d->mRemovedViews.begin();
        DocumentView* newView = *d->mAddedViews.begin();

        newView->setGeometry(rect());
        QPropertyAnimation* anim = newView->fadeIn();

        oldView->setZValue(-1);
        connect(anim, &QPropertyAnimation::finished, oldView, &DocumentView::hideAndDeleteLater);
        d->mRemovedViews.clear();

        return;
    }

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

        for (DocumentView * view : qAsConst(views)) {
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
                view->setGraphicsEffectOpacity(1);
            }

            ++col;
            if (col == colCount) {
                col = 0;
                ++row;
            }
        }
    }

    // Handle removed views
    if (animated) {
        for (DocumentView* view : qAsConst(d->mRemovedViews)) {
            view->fadeOut();
            QTimer::singleShot(DocumentView::AnimDuration, view, &QObject::deleteLater);
        }
    } else {
        for (DocumentView* view : qAsConst(d->mRemovedViews)) {
            view->deleteLater();
        }
        QMetaObject::invokeMethod(this, "pretendFadeInFinished", Qt::QueuedConnection);
    }
    d->mRemovedViews.clear();
}

void DocumentViewContainer::pretendFadeInFinished()
{
    // Animations are disabled. Pretend all fade ins are finished so that added
    // views are moved to mViews, will modify d->mAddedViews
    const auto currentViews = d->mAddedViews;
    for (DocumentView* view : currentViews) {
        slotFadeInFinished(view);
    }
}

void DocumentViewContainer::slotFadeInFinished(DocumentView* view)
{
    if (!d->mAddedViews.contains(view)) {
        // This can happen if user goes to next image then quickly goes to the
        // next one before the animation is finished.
        return;
    }
    d->mAddedViews.remove(view);
    d->mViews.insert(view);
}

void DocumentViewContainer::slotConfigChanged()
{
    bool currentlyGL = qobject_cast<QGLWidget*>(viewport());
    bool wantGL = GwenviewConfig::animationMethod() == DocumentView::GLAnimation;
    if (currentlyGL != wantGL) {
        setViewport(wantGL ? new QGLWidget() : new QWidget());
    }
}

void DocumentViewContainer::showMessageWidget(QGraphicsWidget* widget, Qt::Alignment align)
{
    DocumentView* view = nullptr;
    if (d->mViews.isEmpty()) {
        GV_RETURN_IF_FAIL(!d->mAddedViews.isEmpty());
        view = *d->mAddedViews.begin();
    } else {
        view = *d->mViews.begin();
    }
    GV_RETURN_IF_FAIL(view);

    widget->setParentItem(view);
    GraphicsWidgetFloater* floater = new GraphicsWidgetFloater(view);
    floater->setChildWidget(widget);
    floater->setAlignment(align);
    widget->show();
    widget->setZValue(1);
}

void DocumentViewContainer::applyPalette(const QPalette& palette)
{
    setPalette(palette);
    for (DocumentView* view : d->mViews | d->mAddedViews) {
        view->setPalette(palette);
    }
}

} // namespace
