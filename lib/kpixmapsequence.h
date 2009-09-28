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

#ifndef _K_PIXMAPSEQUENCE_H_
#define _K_PIXMAPSEQUENCE_H_

#include <QtCore/QSharedDataPointer>

#include "gwenviewlib_export.h"

class QPixmap;
class QSize;

/**
 * \class GvPixmapSequence kpixmapsequence.h GvPixmapSequence
 *
 * \brief Loads and gives access to the frames of a typical multi-row pixmap
 * as often used for spinners.
 *
 * GvPixmapSequence is implicitly shared. Copying is fast.
 *
 * Once typically uses the static methods loadFromPath and loadFromPixmap
 * to create an instance of GvPixmapSequence.
 *
 * \author Aurélien Gâteau <agateau@kde.org><br/>Sebastian Trueg <trueg@kde.org>
 */
class GWENVIEWLIB_EXPORT GvPixmapSequence
{
public:
    /**
     * Create an empty sequence
     */
	GvPixmapSequence();

    /**
     * Copy constructor
     */
	GvPixmapSequence( const GvPixmapSequence& other );

    /**
     * Destructor
     */
	~GvPixmapSequence();

    /**
     * Create a copy of \p other. The data is implicitly shared.
     */
    GvPixmapSequence& operator=( const GvPixmapSequence& other );

    /**
     * \return \p true if a sequence was loaded successfully.
     *
     * \sa isEmpty
     */
    bool isValid() const;

    /**
     * \return \p true if no sequence was loaded successfully.
     *
     * \sa isValid
     */
    bool isEmpty() const;

    /**
     * Load a pixmap sequence from \p path.
     *
     * \return \p true if loading was sucessful, \p false otherwise.
     */
	bool load( const QString& path );

    /**
     * Load a pixmap sequence from \p pixmap which contains a set of
     * frames in one column.
     *
     * \return \p true if loading was sucessful, \p false otherwise.
     */
    bool load( const QPixmap& pixmap );

    /**
     * \return The size of an individual frame in the sequence.
     * Be aware that frames are always taken to be squared.
     */
	QSize frameSize() const;

    /**
     * The number of frames in this sequence.
     */
	int frameCount() const;

    /**
     * Retrieve the frame at \p index.
     *
     * \param index The index of the frame in question starting at 0.
     * \p index greater than frameCount() is perfectly valid. The
     * sequence will simply start over.
     */
	QPixmap frameAt( int index ) const;

    static GvPixmapSequence loadFromPath( const QString& path );
    static GvPixmapSequence loadFromPixmap( const QPixmap& path );

private:
    class Private;
	QSharedDataPointer<Private> d;
};

#endif
