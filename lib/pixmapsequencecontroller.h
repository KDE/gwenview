// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#ifndef PIXMAPSEQUENCECONTROLLER_H
#define PIXMAPSEQUENCECONTROLLER_H

// Qt
#include <QObject>

// KDE

// Local

class QPixmap;

class KPixmapSequence;

namespace Gwenview {

class PixmapSequenceControllerPrivate;

/**
 * This class uses an instance of KPixmapSequence and emits the frameChanged()
 * periodically
 */
class PixmapSequenceController : public QObject {
	Q_OBJECT
public:
	PixmapSequenceController(QObject* parent = 0);
	~PixmapSequenceController();

	void setPixmapSequence(const KPixmapSequence&);

	void setInterval(int);

	void start();

	void stop();

Q_SIGNALS:
	void frameChanged(const QPixmap&);

private:
	friend class PixmapSequenceControllerPrivate;
	PixmapSequenceControllerPrivate* const d;

	Q_PRIVATE_SLOT(d, void slotTimerTimeout())
};


} // namespace

#endif /* PIXMAPSEQUENCECONTROLLER_H */
