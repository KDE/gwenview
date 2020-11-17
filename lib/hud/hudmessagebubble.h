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
#ifndef HUDMESSAGEBUBBLE_H
#define HUDMESSAGEBUBBLE_H

#include <lib/gwenviewlib_export.h>

// Qt

// KF

// Local
#include <lib/hud/hudwidget.h>

class KGuiItem;

namespace Gwenview
{
class HudButton;

struct HudMessageBubblePrivate;
/**
 * Shows a bubble with a QLabel and optional buttons.
 * Automatically goes away after a while.
 */
class GWENVIEWLIB_EXPORT HudMessageBubble : public HudWidget
{
    Q_OBJECT
public:
    explicit HudMessageBubble(QGraphicsWidget* parent = nullptr);
    ~HudMessageBubble() override;

    void setText(const QString& text);

    HudButton* addButton(const KGuiItem&);

private:
    HudMessageBubblePrivate* const d;
};

} // namespace

#endif /* HUDMESSAGEBUBBLE_H */
