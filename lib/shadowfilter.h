// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef SHADOWFILTER_H
#define SHADOWFILTER_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt
#include <QObject>

class QColor;
class QWidget;

namespace Gwenview
{

struct ShadowFilterPrivate;
/**
 * Paint shadows on widget edges
 */
class GWENVIEWLIB_EXPORT ShadowFilter : public QObject
{
    Q_OBJECT
public:
    enum WidgetEdge {
        LeftEdge,
        TopEdge,
        RightEdge,
        BottomEdge
    };
    explicit ShadowFilter(QWidget* parent);
    ~ShadowFilter() override;

    void setShadow(WidgetEdge edge, const QColor& color);
    void reset();

protected:
    bool eventFilter(QObject*, QEvent*) override;

private:
    ShadowFilterPrivate* const d;
};

} // namespace

#endif /* SHADOWFILTER_H */
