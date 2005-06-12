// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef CURSORTRACKER_H
#define CURSORTRACKER_H   

// Qt
#include <qlabel.h>

namespace Gwenview {

/**
 * This class implements a decoration-less window which will follow the cursor
 * when it's over a specified widget.
 */
class CursorTracker : public QLabel {
public:
	CursorTracker(const QString& txt, QWidget* reference);

	void setText(const QString& txt);

protected:
	bool eventFilter(QObject*, QEvent*);
};


/**
 * A specialized CursorTracker class, which looks like a tool tip.
 */
class TipTracker : public CursorTracker {
public:
	TipTracker(const QString& txt, QWidget* reference);
};


} // namespace

#endif /* CURSORTRACKER_H */
