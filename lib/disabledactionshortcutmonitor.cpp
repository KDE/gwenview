// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2014 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
// Self
#include <disabledactionshortcutmonitor.h>

// Local

// KF

// Qt
#include <QAction>
#include <QEvent>
#include <QShortcut>
#include <QWidget>

namespace Gwenview
{
class DisabledActionShortcutMonitorPrivate
{
public:
    QShortcut *mShortcut = nullptr;
};
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
DisabledActionShortcutMonitor::DisabledActionShortcutMonitor(QAction *action, QWidget *parent)
#else
DisabledActionShortcutMonitor::DisabledActionShortcutMonitor(QAction *action, QObject *parent)
#endif
    : QObject(parent)
    , d(new DisabledActionShortcutMonitorPrivate)
{
    d->mShortcut = new QShortcut(parent);
    connect(d->mShortcut, &QShortcut::activated, this, &DisabledActionShortcutMonitor::activated);
    action->installEventFilter(this);
}

DisabledActionShortcutMonitor::~DisabledActionShortcutMonitor()
{
    delete d->mShortcut;
    delete d;
}

bool DisabledActionShortcutMonitor::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::ActionChanged) {
        auto action = static_cast<QAction *>(object);
        if (action->isEnabled()) {
            // Unset the shortcut otherwise we get a dialog complaining about
            // ambiguous shortcuts when the user tries to trigger the action
            d->mShortcut->setKey(QKeySequence());
        } else {
            d->mShortcut->setKey(action->shortcut());
        }
    }
    return false;
}

} // namespace
