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

#include "kpixmapsequencewidget.h"
#include "kpixmapsequenceoverlaypainter.h"
#include "kpixmapsequence.h"

#include <KDebug>


class GvPixmapSequenceWidget::Private
{
public:
    GvPixmapSequenceOverlayPainter m_painter;
};


GvPixmapSequenceWidget::GvPixmapSequenceWidget( QWidget* parent )
    : QWidget( parent ),
      d( new Private )
{
    d->m_painter.setWidget(this);
    setSequence(d->m_painter.sequence());
}


GvPixmapSequenceWidget::~GvPixmapSequenceWidget()
{
    delete d;
}


GvPixmapSequence GvPixmapSequenceWidget::sequence() const
{
    return d->m_painter.sequence();
}


int GvPixmapSequenceWidget::interval() const
{
    return d->m_painter.interval();
}


void GvPixmapSequenceWidget::setSequence( const GvPixmapSequence& seq )
{
    setFixedSize(seq.frameSize());
    d->m_painter.setSequence( seq );
    if (isVisible()) {
        d->m_painter.start();
    }
}


void GvPixmapSequenceWidget::setInterval( int msecs )
{
    d->m_painter.setInterval( msecs );
}


void GvPixmapSequenceWidget::showEvent( QShowEvent* event )
{
    d->m_painter.start();
    QWidget::showEvent( event );
}


void GvPixmapSequenceWidget::hideEvent( QHideEvent* event )
{
    d->m_painter.stop();
    QWidget::hideEvent( event );
}

void hideEvent( QHideEvent* );

#include "kpixmapsequencewidget.moc"
