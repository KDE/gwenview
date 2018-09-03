// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#ifndef ZOOMSLIDER_H
#define ZOOMSLIDER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QWidget>

// KDE

// Local

class QAction;
class QSlider;

namespace Gwenview
{

struct ZoomSliderPrivate;
/**
 * A widget featuring an horizontal slider and zoom in/out buttons.
 * By default zoom in/out buttons will trigger SliderPageStep{Add,Sub}.
 * Their behavior can be changed with setZoomInAction() and
 * setZoomOutAction().
 */
class GWENVIEWLIB_EXPORT ZoomSlider : public QWidget
{
    Q_OBJECT
public:
    explicit ZoomSlider(QWidget* parent = nullptr);
    ~ZoomSlider() override;

    int value() const;

    QSlider* slider() const;

    void setZoomInAction(QAction*);

    void setZoomOutAction(QAction*);

public Q_SLOTS:
    void setValue(int);

    void setMinimum(int);

    void setMaximum(int);

Q_SIGNALS:
    int valueChanged(int);

private Q_SLOTS:
    void slotActionTriggered(int actionTriggered);
    void zoomOut();
    void zoomIn();

private:
    ZoomSliderPrivate* const d;
};

} // namespace

#endif /* ZOOMSLIDER_H */
