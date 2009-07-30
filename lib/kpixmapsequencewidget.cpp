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


class KPixmapSequenceWidget::Private
{
public:
    KPixmapSequenceOverlayPainter m_painter;
};


KPixmapSequenceWidget::KPixmapSequenceWidget( QWidget* parent )
    : QWidget( parent ),
      d( new Private )
{
    d->m_painter.setWidget(this);
    setSequence(d->m_painter.sequence());
}


KPixmapSequenceWidget::~KPixmapSequenceWidget()
{
    delete d;
}


KPixmapSequence KPixmapSequenceWidget::sequence() const
{
    return d->m_painter.sequence();
}


int KPixmapSequenceWidget::interval() const
{
    return d->m_painter.interval();
}


void KPixmapSequenceWidget::setSequence( const KPixmapSequence& seq )
{
    setFixedSize(seq.frameSize());
    d->m_painter.setSequence( seq );
    d->m_painter.start();
}


void KPixmapSequenceWidget::setInterval( int msecs )
{
    d->m_painter.setInterval( msecs );
}


void KPixmapSequenceWidget::start()
{
    d->m_painter.start();
}


void KPixmapSequenceWidget::stop()
{
    d->m_painter.stop();
}

#include "kpixmapsequencewidget.moc"
