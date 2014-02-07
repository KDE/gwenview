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
#ifndef HUDBUTTONBOX_H
#define HUDBUTTONBOX_H

#include <lib/gwenviewlib_export.h>

// Local
#include <lib/graphicshudwidget.h>

// KDE

// Qt

class QAction;
class QGraphicsWidget;

namespace Gwenview
{

class GraphicsHudButton;

class HudButtonBoxPrivate;
/**
 * A hud widget which shows a list of buttons
 */
class GWENVIEWLIB_EXPORT HudButtonBox : public GraphicsHudWidget
{
    Q_OBJECT
public:
    HudButtonBox(QGraphicsWidget* parent = 0);
    ~HudButtonBox();

    void setText(const QString& text);

    GraphicsHudButton* addButton(const QString& text);

    GraphicsHudButton* addAction(QAction* action, const QString& overrideText = QString());

protected:
    void showEvent(QShowEvent* event);

private:
    HudButtonBoxPrivate* const d;
};

} // namespace

#endif /* HUDBUTTONBOX_H */
