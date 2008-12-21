// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
// Self
#include "imagesequence.moc"

// Qt
#include <QPainter>
#include <QPixmap>

// KDE
#include <kdebug.h>

// Local

namespace Gwenview {


struct ImageSequencePrivate {
	QVector<QPixmap> mFrames;
};


ImageSequence::ImageSequence(QObject* parent)
: QObject(parent)
, d(new ImageSequencePrivate) {
}


ImageSequence::~ImageSequence() {
	delete d;
}


bool ImageSequence::load(const QString& path) {
	QPixmap bigPixmap;
	if (!bigPixmap.load(path)) {
		kWarning() << "Could not load animation image from" << path;
		return false;
	}

	d->mFrames.resize(bigPixmap.height() / bigPixmap.width());
	const int size = bigPixmap.width();

	for(int pos=0; pos < d->mFrames.size(); ++pos) {
		QPixmap pix = QPixmap(size, size);
		pix.fill(Qt::transparent);
		QPainter painter(&pix);
		painter.drawPixmap(QPoint(0, 0), bigPixmap, QRect(0, pos * size, size, size));
		d->mFrames[pos] = pix;
	}
	return true;
}


QSize ImageSequence::frameSize() const {
	if (d->mFrames.size() == 0) {
		kWarning() << "No frame loaded";
		return QSize();
	}
	return d->mFrames[0].size();
}


int ImageSequence::frameCount() const {
	return d->mFrames.size();
}


QPixmap ImageSequence::frameAt(int index) const {
	if (index >= d->mFrames.size()) {
		kWarning() << "index is too high" << index;
		return QPixmap();
	}
	return d->mFrames.at(index);
}



} // namespace
