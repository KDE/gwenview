// vim:set tabstop=4 shiftwidth=4 noexpandtab:
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

#ifndef GVFULLSCREENBAR_H
#define GVFULLSCREENBAR_H   

// Qt
#include <qlabel.h>

// KDE
#include <kaction.h>

class QResizeEvent;
class QString;

class GVFullScreenBar : public QLabel {
Q_OBJECT
public:
	GVFullScreenBar(QWidget* parent, const KActionPtrList& actions);
	~GVFullScreenBar();
	void resizeEvent(QResizeEvent* event);
	void setText(const QString& text);

	void slideIn();
	void slideOut();

private slots:
	void slotUpdateSlide();

private:
	class Private;
	Private* d;
};

#endif /* GVFULLSCREENBAR_H */
