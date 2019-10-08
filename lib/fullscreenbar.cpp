// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "fullscreenbar.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QBitmap>
#include <QEvent>
#include <QMouseEvent>
#include <QLayout>
#include <QTimeLine>
#include <QTimer>
#include <QToolButton>
#include <QScreen>

// KDE
#include <KLocalizedString>

// Local

namespace Gwenview
{

static const int SLIDE_DURATION = 150;
static const int AUTO_HIDE_CURSOR_TIMEOUT = 1000;

// How long before the bar slide out after switching to fullscreen
static const int INITIAL_HIDE_TIMEOUT = 2000;

// Do not slide bar out if mouse is less than this amount of pixels below bar, to
// prevent accidental slide outs
static const int EXTRA_BAR_HEIGHT = 20;

struct FullScreenBarPrivate
{
    FullScreenBar* q;
    QTimeLine* mTimeLine;
    QTimer* mAutoHideCursorTimer;
    bool mAutoHidingEnabled;
    bool mEdgeTriggerEnabled;
    QTimer* mInitialHideTimer;

    void startTimeLine()
    {
        if (mTimeLine->state() != QTimeLine::Running) {
            mTimeLine->start();
        }
    }

    void hideCursor()
    {
        QBitmap empty(32, 32);
        empty.clear();
        QCursor blankCursor(empty, empty);
        QApplication::setOverrideCursor(blankCursor);
    }

    /**
     * Returns the rectangle in which the mouse must enter to trigger bar
     * sliding. The rectangle is in global coords.
     */
    QRect slideInTriggerRect() const
    {
        QRect rect = QGuiApplication::screenAt(QCursor::pos())->geometry();
        // Take parent widget position into account because it may not be at
        // the top of the screen, for example when the save bar warning is
        // shown.
        rect.setHeight(q->parentWidget()->y() + q->height() + EXTRA_BAR_HEIGHT);
        return rect;
    }

    bool shouldHide() const
    {
        Q_ASSERT(q->parentWidget());

        if (!mAutoHidingEnabled) {
            return false;
        }
        if (slideInTriggerRect().contains(QCursor::pos())) {
            return false;
        }
        if (QApplication::activePopupWidget()) {
            return false;
        }
        // Do not hide if a button is down, which can happen when we are
        // using a scroll bar.
        if (QApplication::mouseButtons() != Qt::NoButton) {
            return false;
        }
        return true;
    }
};

FullScreenBar::FullScreenBar(QWidget* parent)
: QFrame(parent)
, d(new FullScreenBarPrivate)
{
    d->q = this;
    d->mAutoHidingEnabled = true;
    d->mEdgeTriggerEnabled = true;
    setObjectName(QStringLiteral("fullScreenBar"));

    d->mTimeLine = new QTimeLine(SLIDE_DURATION, this);
    connect(d->mTimeLine, &QTimeLine::valueChanged, this, &FullScreenBar::moveBar);

    d->mAutoHideCursorTimer = new QTimer(this);
    d->mAutoHideCursorTimer->setInterval(AUTO_HIDE_CURSOR_TIMEOUT);
    d->mAutoHideCursorTimer->setSingleShot(true);
    connect(d->mAutoHideCursorTimer, &QTimer::timeout, this, &FullScreenBar::slotAutoHideCursorTimeout);

    d->mInitialHideTimer = new QTimer(this);
    d->mInitialHideTimer->setInterval(INITIAL_HIDE_TIMEOUT);
    d->mInitialHideTimer->setSingleShot(true);
    connect(d->mInitialHideTimer, &QTimer::timeout, this, &FullScreenBar::slideOut);

    hide();
}

FullScreenBar::~FullScreenBar()
{
    delete d;
}

QSize FullScreenBar::sizeHint() const
{
    QSize sh = QFrame::sizeHint();
    if (!layout()) {
        return sh;
    }

    if (layout()->expandingDirections() & Qt::Horizontal) {
        sh.setWidth(parentWidget()->width());
    }
    return sh;
}

void FullScreenBar::moveBar(qreal value)
{
    move(0, -height() + int(value * height()));

    // For some reason, if Gwenview is started with command line options to
    // start a slideshow, the bar might end up below the view. Calling raise()
    // here fixes it.
    raise();
}

void FullScreenBar::setActivated(bool activated)
{
    if (activated) {
        // Delay installation of event filter because switching to fullscreen
        // cause a few window adjustments, which seems to generate unwanted
        // mouse events, which cause the bar to slide in.
        QTimer::singleShot(500, this, &FullScreenBar::delayedInstallEventFilter);

        adjustSize();

        // Make sure the widget is visible on start
        move(0, 0);
        raise();
        show();
    } else {
        qApp->removeEventFilter(this);
        hide();
        d->mAutoHideCursorTimer->stop();
        QApplication::restoreOverrideCursor();
    }
}

void FullScreenBar::delayedInstallEventFilter()
{
    qApp->installEventFilter(this);
    if (d->shouldHide()) {
        d->mInitialHideTimer->start();
        d->hideCursor();
    }
}

void FullScreenBar::slotAutoHideCursorTimeout()
{
    if (d->shouldHide()) {
        d->hideCursor();
    } else {
        d->mAutoHideCursorTimer->start();
    }
}

void FullScreenBar::slideOut()
{
    d->mInitialHideTimer->stop();
    d->mTimeLine->setDirection(QTimeLine::Backward);
    d->startTimeLine();
}

void FullScreenBar::slideIn()
{
    d->mInitialHideTimer->stop();
    d->mTimeLine->setDirection(QTimeLine::Forward);
    d->startTimeLine();
}

bool FullScreenBar::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        QApplication::restoreOverrideCursor();
        d->mAutoHideCursorTimer->start();
        if (y() == 0) {
            if (d->shouldHide()) {
                slideOut();
            }
        } else {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent *>(event);
            if (d->mEdgeTriggerEnabled && mouseEvent->buttons() == 0 && d->slideInTriggerRect().contains(QCursor::pos())) {
                slideIn();
            }
        }
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        // This can happen if user released the mouse after using a scrollbar
        // in the content (the bar does not hide while a button is down)
        if (y() == 0 && d->shouldHide()) {
            slideOut();
        }
        return false;
    }

    // Filtering message on tooltip text for CJK to remove accelerators.
    // Quoting ktoolbar.cpp:
    // """
    // CJK languages use more verbose accelerator marker: they add a Latin
    // letter in parenthesis, and put accelerator on that. Hence, the default
    // removal of ampersand only may not be enough there, instead the whole
    // parenthesis construct should be removed. Use KLocale's method to do this.
    // """
    if (event->type() == QEvent::Show || event->type() == QEvent::Paint) {
        QToolButton* button = qobject_cast<QToolButton*>(object);
        if (button && !button->actions().isEmpty()) {
            QAction* action = button->actions().constFirst();
            QString toolTip = KLocalizedString::removeAcceleratorMarker(action->toolTip());
            // Filtering message requested by translators (scripting).
            button->setToolTip(i18nc("@info:tooltip of custom toolbar button", "%1", toolTip));
        }
    }

    return false;
}

void FullScreenBar::setAutoHidingEnabled(bool value)
{
    d->mAutoHidingEnabled = value;
}

void FullScreenBar::setEdgeTriggerEnabled(bool value)
{
    d->mEdgeTriggerEnabled = value;
}

} // namespace
