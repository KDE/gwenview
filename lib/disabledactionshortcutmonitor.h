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
#ifndef DISABLEDACTIONSHORTCUTMONITOR_H
#define DISABLEDACTIONSHORTCUTMONITOR_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt
#include <QObject>

class QAction;

namespace Gwenview
{

class DisabledActionShortcutMonitorPrivate;
/**
 * Monitors an action and emits a signal if one tries to trigger the action
 * using its shortcut while it is disabled.
 */
class GWENVIEWLIB_EXPORT DisabledActionShortcutMonitor : public QObject
{
    Q_OBJECT
public:
    /**
     * parent must be a widget because we need to create a QShortcut
     */
    DisabledActionShortcutMonitor(QAction* action, QWidget* parent);
    ~DisabledActionShortcutMonitor() Q_DECL_OVERRIDE;

Q_SIGNALS:
    void activated();

protected:
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

private:
    DisabledActionShortcutMonitorPrivate* const d;
};

} // namespace

#endif /* DISABLEDACTIONSHORTCUTMONITOR_H */
