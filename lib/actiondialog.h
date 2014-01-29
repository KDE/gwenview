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
#ifndef ACTIONDIALOG_H
#define ACTIONDIALOG_H

#include "gwenviewlib_export.h"

// Local

// KDE
#include <KDialog>

// Qt

class QAction;
class QPushButton;

namespace Gwenview
{

class ActionDialogPrivate;
/**
 * A dialog which shows a list of actions
 */
class GWENVIEWLIB_EXPORT ActionDialog : public KDialog
{
    Q_OBJECT
public:
    ActionDialog(QWidget* parent = 0);
    ~ActionDialog();

    void setText(const QString& text);

    QPushButton* addButton(const QString& text);

    QPushButton* addAction(QAction* action, const QString& overrideText = QString());

private:
    ActionDialogPrivate* const d;
};

} // namespace

#endif /* ACTIONDIALOG_H */
