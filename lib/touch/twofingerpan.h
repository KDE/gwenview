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
#ifndef TWOFINGERPAN_H
#define TWOFINGERPAN_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QGesture>
#include <QGestureRecognizer>

// KF

// Local

namespace Gwenview
{
struct TwoFingerPanRecognizerPrivate;

class GWENVIEWLIB_EXPORT TwoFingerPan : public QGesture
{
    Q_PROPERTY(QPointF delta READ getDelta WRITE setDelta)
    Q_PROPERTY(bool delayActive READ getDelayActive WRITE setDelayActive)

public:
    explicit TwoFingerPan(QObject *parent = nullptr);
    QPointF getDelta()
    {
        return delta;
    };
    void setDelta(QPointF _delta)
    {
        delta = _delta;
    };
    bool getDelayActive()
    {
        return delayActive;
    };
    void setDelayActive(bool _delay)
    {
        delayActive = _delay;
    };

private:
    QPointF delta;
    bool delayActive;
};

class GWENVIEWLIB_EXPORT TwoFingerPanRecognizer : public QGestureRecognizer
{
public:
    explicit TwoFingerPanRecognizer();
    ~TwoFingerPanRecognizer() override;

private:
    TwoFingerPanRecognizerPrivate *d;

    QGesture *create(QObject *) override;
    Result recognize(QGesture *, QObject *, QEvent *) override;
};

} // namespace
#endif /* TWOFINGERPAN_H */
