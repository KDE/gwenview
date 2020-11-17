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
#ifndef ONEANDTWOFINGERSWIPE_H
#define ONEANDTWOFINGERSWIPE_H

#include <lib/gwenviewlib_export.h>
// Qt
#include <QGesture>
#include <QGestureRecognizer>

// KDE

// Local

namespace Gwenview
{
struct OneAndTwoFingerSwipeRecognizerPrivate;

class GWENVIEWLIB_EXPORT OneAndTwoFingerSwipe : public QGesture
{
    Q_PROPERTY(bool left READ left WRITE left)
    Q_PROPERTY(bool right READ right WRITE right)

public:
    explicit OneAndTwoFingerSwipe(QObject* parent = nullptr);

private:
    bool left;
    bool right;
};

class GWENVIEWLIB_EXPORT OneAndTwoFingerSwipeRecognizer : public QGestureRecognizer
{
public:
    explicit OneAndTwoFingerSwipeRecognizer();
    ~OneAndTwoFingerSwipeRecognizer();
private:
    OneAndTwoFingerSwipeRecognizerPrivate* d;

    virtual QGesture* create(QObject*) override;
    virtual Result recognize(QGesture*, QObject*, QEvent*) override;
};

} // namespace
#endif /* ONEANDTWOFINGERSWIPE_H */
