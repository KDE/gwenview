/*
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

#ifndef _K_PIXMAPSEQUENCE_WIDGET_H_
#define _K_PIXMAPSEQUENCE_WIDGET_H_

#include <QtGui/QWidget>

#include "gwenviewlib_export.h"

class KPixmapSequence;

/**
 * \class KPixmapSequenceWidget kpixmapsequencewidget.h KPixmapSequenceWidget
 *
 * \brief A simple widget showing a fixed size pixmap sequence.
 *
 * The KPixmapSequenceWidget uses the KPixmapSequenceOverlayPainter to show a
 * sequence of pixmaps. It is intended as a simple wrapper around the
 * KPixmapSequenceOverlayPainter in case a widget is more appropriate than
 * an event filter.
 *
 * \author Sebastian Trueg <trueg@kde.org>
 *
 * \since 4.4
 */
class GWENVIEWLIB_EXPORT KPixmapSequenceWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constructor
     */
    KPixmapSequenceWidget( QWidget* parent = 0 );

    /**
     * Destructor
     */
    ~KPixmapSequenceWidget();

    /**
     * The sequenece used to draw the overlay.
     *
     * \sa setSequence
     */
    KPixmapSequence sequence() const;

    /**
     * The interval between frames.
     *
     * \sa setInterval, KPixmapSequenceOverlayPainter::interval
     */
    int interval() const;

public Q_SLOTS:
    /**
     * Set the sequence to be used. By default the KDE busy sequence is used.
     */
    void setSequence( const KPixmapSequence& seq );

    /**
     * Set the interval between frames. The default is 200.
     * \sa interval, KPixmapSequenceOverlayPainter::setInterval
     */
    void setInterval( int msecs );

protected:
    virtual void showEvent( QShowEvent* );

    virtual void hideEvent( QHideEvent* );

private:
    class Private;
    Private* const d;
};

#endif
