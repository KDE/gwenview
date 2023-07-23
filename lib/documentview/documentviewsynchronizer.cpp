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
#include "documentviewsynchronizer.h"

// Local
#include <documentview/documentview.h>

// KF

// Qt
#include <QPointer>

namespace Gwenview
{
struct DocumentViewSynchronizerPrivate {
    DocumentViewSynchronizer *q = nullptr;
    const QList<DocumentView *> *mViews = nullptr;
    QPointer<DocumentView> mCurrentView;
    bool mActive;
    QPoint mOldPosition;

    DocumentViewSynchronizerPrivate(const QList<DocumentView *> *views)
        : mViews(views)
    {
    }

    void updateConnections()
    {
        if (!mCurrentView || !mActive) {
            return;
        }

        QObject::connect(mCurrentView.data(), SIGNAL(zoomChanged(qreal)), q, SLOT(setZoom(qreal)));
        QObject::connect(mCurrentView.data(), SIGNAL(zoomToFitChanged(bool)), q, SLOT(setZoomToFit(bool)));
        QObject::connect(mCurrentView.data(), SIGNAL(zoomToFillChanged(bool)), q, SLOT(setZoomToFill(bool)));
        QObject::connect(mCurrentView.data(), SIGNAL(positionChanged()), q, SLOT(updatePosition()));

        for (DocumentView *view : qAsConst(*mViews)) {
            if (view == mCurrentView.data()) {
                continue;
            }
            view->setZoom(mCurrentView.data()->zoom());
            view->setZoomToFit(mCurrentView.data()->zoomToFit());
            view->setZoomToFill(mCurrentView.data()->zoomToFill());
        }
    }

    void updateOldPosition()
    {
        if (!mCurrentView || !mActive) {
            return;
        }
        mOldPosition = mCurrentView.data()->position();
    }
};

DocumentViewSynchronizer::DocumentViewSynchronizer(const QList<DocumentView *> *views, QObject *parent)
    : QObject(parent)
    , d(new DocumentViewSynchronizerPrivate(views))
{
    d->q = this;
    d->mActive = false;
}

DocumentViewSynchronizer::~DocumentViewSynchronizer()
{
    delete d;
}

void DocumentViewSynchronizer::setCurrentView(DocumentView *view)
{
    if (d->mCurrentView) {
        disconnect(d->mCurrentView.data(), nullptr, this, nullptr);
    }
    d->mCurrentView = view;
    d->updateOldPosition();
    d->updateConnections();
}

void DocumentViewSynchronizer::setActive(bool active)
{
    d->mActive = active;
    d->updateOldPosition();
    d->updateConnections();
}

void DocumentViewSynchronizer::setZoom(qreal zoom)
{
    for (DocumentView *view : qAsConst(*d->mViews)) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setZoom(zoom);
    }
    d->updateOldPosition();
}

void DocumentViewSynchronizer::setZoomToFit(bool fit)
{
    for (DocumentView *view : qAsConst(*d->mViews)) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setZoomToFit(fit);
    }
    d->updateOldPosition();
}

void DocumentViewSynchronizer::setZoomToFill(bool fit)
{
    for (DocumentView *view : qAsConst(*d->mViews)) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setZoomToFill(fit);
    }
    d->updateOldPosition();
}

void DocumentViewSynchronizer::updatePosition()
{
    QPoint pos = d->mCurrentView.data()->position();
    QPoint delta = pos - d->mOldPosition;
    d->mOldPosition = pos;
    for (DocumentView *view : qAsConst(*d->mViews)) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setPosition(view->position() + delta);
    }
}

} // namespace

#include "moc_documentviewsynchronizer.cpp"
