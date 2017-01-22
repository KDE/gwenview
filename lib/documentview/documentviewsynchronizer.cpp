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

// KDE

// Qt
#include <QPointer>

namespace Gwenview
{

struct DocumentViewSynchronizerPrivate
{
    DocumentViewSynchronizer* q;
    const QList<DocumentView*>* mViews;
    QPointer<DocumentView> mCurrentView;
    bool mActive;
    QPoint mOldPosition;

    DocumentViewSynchronizerPrivate(const QList<DocumentView*>* views)
    : mViews(views)
    {}

    void updateConnections()
    {
        if (!mCurrentView || !mActive) {
            return;
        }

        QObject::connect(mCurrentView.data(), SIGNAL(zoomChanged(qreal)),
                         q, SLOT(setZoom(qreal)));
        QObject::connect(mCurrentView.data(), SIGNAL(zoomToFitChanged(bool)),
                         q, SLOT(setZoomToFit(bool)));
        QObject::connect(mCurrentView.data(), SIGNAL(positionChanged()),
                         q, SLOT(updatePosition()));

        Q_FOREACH(DocumentView* view, *mViews) {
            if (view == mCurrentView.data()) {
                continue;
            }
            view->setZoom(mCurrentView.data()->zoom());
            view->setZoomToFit(mCurrentView.data()->zoomToFit());
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

DocumentViewSynchronizer::DocumentViewSynchronizer(const QList<DocumentView*>* views, QObject* parent)
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

void DocumentViewSynchronizer::setCurrentView(DocumentView* view)
{
    if (d->mCurrentView) {
        disconnect(d->mCurrentView.data(), 0, this, 0);
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
    Q_FOREACH(DocumentView* view, *d->mViews) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setZoom(zoom);
    }
    d->updateOldPosition();
}

void DocumentViewSynchronizer::setZoomToFit(bool fit)
{
    Q_FOREACH(DocumentView* view, *d->mViews) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setZoomToFit(fit);
    }
    d->updateOldPosition();
}

void DocumentViewSynchronizer::updatePosition()
{
    QPoint pos = d->mCurrentView.data()->position();
    QPoint delta = pos - d->mOldPosition;
    d->mOldPosition = pos;
    Q_FOREACH(DocumentView* view, *d->mViews) {
        if (view == d->mCurrentView.data()) {
            continue;
        }
        view->setPosition(view->position() + delta);
    }
}

} // namespace
