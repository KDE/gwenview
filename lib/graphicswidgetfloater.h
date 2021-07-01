// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef GRAPHICSWIDGETFLOATER_H
#define GRAPHICSWIDGETFLOATER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>

// KF

// Local

class QGraphicsWidget;

namespace Gwenview
{
struct GraphicsWidgetFloaterPrivate;

/**
 * This helper object makes it possible to place a widget (the child) over
 * another (the parent), ensuring the child remains aligned as specified by
 * setAlignment() whenever either widget get resized.
 */
class GWENVIEWLIB_EXPORT GraphicsWidgetFloater : public QObject
{
    Q_OBJECT
public:
    GraphicsWidgetFloater(QGraphicsWidget *parent);
    ~GraphicsWidgetFloater() override;

    void setChildWidget(QGraphicsWidget *);

    void setAlignment(Qt::Alignment);

    void setHorizontalMargin(int);
    int horizontalMargin() const;

    void setVerticalMargin(int);
    int verticalMargin() const;

protected:
    bool eventFilter(QObject *, QEvent *) override;

private Q_SLOTS:
    void slotChildVisibilityChanged();

private:
    GraphicsWidgetFloaterPrivate *const d;
};

} // namespace

#endif /* GRAPHICSWIDGETFLOATER_H */
