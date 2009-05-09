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
#ifndef IMAGESEQUENCE_H
#define IMAGESEQUENCE_H

// Qt
#include <QObject>

// KDE

// Local

class QPixmap;
class QSize;

namespace Gwenview {


class ImageSequencePrivate;
/**
 * This class loads an animation defined has an image containing all frames in
 * a column
 */
class ImageSequence : public QObject {
	Q_OBJECT
public:
	ImageSequence(QObject* parent = 0);
	~ImageSequence();

	bool load(const QString& path);

	QSize frameSize() const;

	int frameCount() const;

	QPixmap frameAt(int) const;

private:
	ImageSequencePrivate* const d;
};


} // namespace

#endif /* IMAGESEQUENCE_H */
