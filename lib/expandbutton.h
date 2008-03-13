/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef EXPANDBUTTON_H
#define EXPANDBUTTON_H

#include <lib/gwenviewlib_export.h>

#include <QToolButton>


namespace Gwenview {


/**
 * A button which looks flat and shows an expand indicator ([-] or [+]).
 *
 * To implement expandable frames, add a button like this above your frame,
 * connect the toggled(bool) signal of the button to the setVisible(bool) slot
 * of the frame.
 * By default the button is not on, so be sure to hide the frame.
 */
class GWENVIEWLIB_EXPORT ExpandButton : public QToolButton {
public:
	ExpandButton(QWidget* parent);
	virtual QSize sizeHint() const;

protected:
	virtual void paintEvent(QPaintEvent* event);
};


} // namespace


#endif /* EXPANDBUTTON_H */
