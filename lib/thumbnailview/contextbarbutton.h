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

/*
   *****************************************************
   ******************************************************
   **** NOTE: This class is deprecated. Do not use it. **
   ****   It will be removed in the QT 6 timeframe.    **
   ******************************************************
   ******************************************************
*/


#ifndef CONTEXTBARBUTTON_H
#define CONTEXTBARBUTTON_H

// Local

// KDE

// Qt
#include <QToolButton>

namespace Gwenview
{

struct ContextBarButtonPrivate;
/**
 * A button with a special look, appears when hovering over thumbnails
 */
class ContextBarButton : public QToolButton
{
    Q_OBJECT
public:
    explicit ContextBarButton(const QString& iconName, QWidget* parent = 0);
    ~ContextBarButton();

protected:
    void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

private:
    ContextBarButtonPrivate* const d;
};

} // namespace

#endif /* CONTEXTBARBUTTON_H */
