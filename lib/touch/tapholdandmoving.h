/*
Gwenview: an image viewer
Copyright 2019 Steffen Hartleib <steffenhartleib@t-online.de>

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
#ifndef TAPHOLDANDMOVING_H
#define TAPHOLDANDMOVING_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGesture>
#include <QGestureRecognizer>

// KF

// Local

namespace Gwenview
{
struct TapHoldAndMovingRecognizerPrivate;

class GWENVIEWLIB_EXPORT TapHoldAndMoving : public QGesture
{
    Q_PROPERTY(QPoint pos READ getPos WRITE setPos)

public:
    explicit TapHoldAndMoving(QObject *parent = nullptr);
    QPoint getPos()
    {
        return pos;
    };
    void setPos(QPoint _pos)
    {
        pos = _pos;
    };

private:
    QPoint pos;
};

class GWENVIEWLIB_EXPORT TapHoldAndMovingRecognizer : public QGestureRecognizer
{
public:
    explicit TapHoldAndMovingRecognizer();
    ~TapHoldAndMovingRecognizer() override;

private:
    TapHoldAndMovingRecognizerPrivate *d;

    virtual QGesture *create(QObject *target) override;
    virtual Result recognize(QGesture *state, QObject *watched, QEvent *event) override;
};

} // namespace
#endif /* TAPHOLDANDMOVING_H */
