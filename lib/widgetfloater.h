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
#ifndef WIDGETFLOATER_H
#define WIDGETFLOATER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>

// KDE

// Local

namespace Gwenview
{

struct WidgetFloaterPrivate;

/**
 * This helper object makes it possible to place a widget (the child) over
 * another (the parent), ensuring the child remains aligned as specified by
 * setAlignment() whenever either widget get resized.
 */
class GWENVIEWLIB_EXPORT WidgetFloater : public QObject
{
    Q_OBJECT
public:
    WidgetFloater(QWidget* parent);
    ~WidgetFloater();

    void setChildWidget(QWidget*);

    void setAlignment(Qt::Alignment);

    void setHorizontalMargin(int);
    int horizontalMargin() const;

    void setVerticalMargin(int);
    int verticalMargin() const;

protected:
    bool eventFilter(QObject*, QEvent*) Q_DECL_OVERRIDE;

private:
    WidgetFloaterPrivate* const d;
};

} // namespace

#endif /* WIDGETFLOATER_H */
