/*
  Copyright 2008 Aurélien Gâteau <agateau@kde.org>
  Copyright 2009 Sebastian Trueg <trueg@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "kpixmapsequence.h"

#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtCore/QVector>

#include <kdebug.h>


class KPixmapSequence::Private : public QSharedData
{
public:
	QVector<QPixmap> mFrames;
};


KPixmapSequence::KPixmapSequence()
    : d(new Private)
{
}


KPixmapSequence::KPixmapSequence( const KPixmapSequence& other )
{
    d = other.d;
}


KPixmapSequence::~KPixmapSequence()
{
}


KPixmapSequence& KPixmapSequence::operator=( const KPixmapSequence& other )
{
    d = other.d;
    return *this;
}


bool KPixmapSequence::isValid() const
{
    return !isEmpty();
}


bool KPixmapSequence::isEmpty() const
{
    return d->mFrames.isEmpty();
}


bool KPixmapSequence::load(const QString& path)
{
	QPixmap bigPixmap;
	if (!bigPixmap.load(path)) {
		kWarning() << "Could not load animation image from" << path;
		return false;
	}
    else {
        return load( bigPixmap );
    }
}


bool KPixmapSequence::load(const QPixmap& bigPixmap)
{
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


QSize KPixmapSequence::frameSize() const
{
	if (d->mFrames.size() == 0) {
		kWarning() << "No frame loaded";
		return QSize();
	}
	return d->mFrames[0].size();
}


int KPixmapSequence::frameCount() const
{
	return d->mFrames.size();
}


QPixmap KPixmapSequence::frameAt(int index) const
{
	return d->mFrames.at( index % frameCount() );
}


KPixmapSequence KPixmapSequence::loadFromPath( const QString& path )
{
    KPixmapSequence s;
    s.load( path );
    return s;
}


KPixmapSequence KPixmapSequence::loadFromPixmap( const QPixmap& pix )
{
    KPixmapSequence s;
    s.load( pix );
    return s;
}
