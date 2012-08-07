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
#ifndef ZOOMWIDGET_H
#define ZOOMWIDGET_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QFrame>

// KDE

// Local

class QAction;

namespace Gwenview
{

struct ZoomWidgetPrivate;
class GWENVIEWLIB_EXPORT ZoomWidget : public QFrame
{
    Q_OBJECT
public:
    ZoomWidget(QWidget* parent = 0);
    ~ZoomWidget();

    void setActions(QAction* zoomToFitAction, QAction* actualSizeAction, QAction* zoomInAction, QAction* zoomOutAction);

    bool isZoomLocked() const;
    void setZoomLocked(bool);

    void setLockZoomButtonVisible(bool);

public Q_SLOTS:
    void setZoom(qreal zoom);

    void setMinimumZoom(qreal zoom);
    void setMaximumZoom(qreal zoom);

Q_SIGNALS:
    void zoomChanged(qreal);

private Q_SLOTS:
    void slotZoomSliderActionTriggered();
    void updateLockZoomButton();

private:
    friend struct ZoomWidgetPrivate;
    ZoomWidgetPrivate* const d;
};

} // namespace

#endif /* ZOOMWIDGET_H */
