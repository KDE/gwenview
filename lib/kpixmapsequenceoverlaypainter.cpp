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

#include "kpixmapsequenceoverlaypainter.h"
#include "kpixmapsequence.h"

#include <QtGui/QWidget>
#include <QtCore/QPoint>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtCore/QCoreApplication>

#include <KIconLoader>
#include <KDebug>


class GvPixmapSequenceOverlayPainter::Private
{
public:
    void _k_timeout();
    void paintFrame();

    GvPixmapSequence m_sequence;
    QPointer<QWidget> m_widget;
    Qt::Alignment m_alignment;
    QPoint m_offset;

    QTimer m_timer;
    int m_counter;

    GvPixmapSequenceOverlayPainter* q;
};


void GvPixmapSequenceOverlayPainter::Private::_k_timeout()
{
    ++m_counter;
    m_counter %= m_sequence.frameCount();
    m_widget->update();
}


void GvPixmapSequenceOverlayPainter::Private::paintFrame()
{
    QPainter p( m_widget );

    QPoint pos;
    if ( m_alignment & Qt::AlignHCenter )
        pos.setX( ( m_widget->width() - m_sequence.frameSize().width() ) / 2 );
    else if ( m_alignment & Qt::AlignRight )
        pos.setX( m_widget->contentsRect().right() - m_sequence.frameSize().width() );

    if ( m_alignment & Qt::AlignVCenter )
        pos.setY( ( m_widget->height() - m_sequence.frameSize().height() ) / 2 );
    else if ( m_alignment & Qt::AlignBottom )
        pos.setY( m_widget->contentsRect().bottom() - m_sequence.frameSize().height() );

    pos += m_offset;

    p.drawPixmap( pos, m_sequence.frameAt( m_counter ) );
}


GvPixmapSequenceOverlayPainter::GvPixmapSequenceOverlayPainter( QObject* parent )
    : QObject( parent ),
      d( new Private )
{
    d->q = this;
    d->m_widget = 0;
    d->m_alignment = Qt::AlignCenter;
    setInterval( 200 );
    d->m_sequence.load( KIconLoader::global()->iconPath("process-working", -22 ) );
    connect( &d->m_timer, SIGNAL( timeout() ), this, SLOT( _k_timeout() ) );
}


GvPixmapSequenceOverlayPainter::~GvPixmapSequenceOverlayPainter()
{
    stop();
    delete d;
}


GvPixmapSequence GvPixmapSequenceOverlayPainter::sequence() const
{
    return d->m_sequence;
}


int GvPixmapSequenceOverlayPainter::interval() const
{
    return d->m_timer.interval();
}


void GvPixmapSequenceOverlayPainter::setSequence( const GvPixmapSequence& seq )
{
    d->m_sequence = seq;
}


void GvPixmapSequenceOverlayPainter::setInterval( int msecs )
{
    d->m_timer.setInterval( msecs );
}


void GvPixmapSequenceOverlayPainter::setWidget( QWidget* w )
{
    stop();
    d->m_widget = w;
}


void GvPixmapSequenceOverlayPainter::setPosition( Qt::Alignment align, const QPoint& offset )
{
    d->m_alignment = align;
    d->m_offset = offset;
}


void GvPixmapSequenceOverlayPainter::start()
{
    if ( d->m_widget ) {
        stop();

        d->m_counter = 0;
        d->m_widget->installEventFilter( this );
        d->m_timer.start();
        d->m_widget->update();
    }
}


void GvPixmapSequenceOverlayPainter::stop()
{
    d->m_timer.stop();
    if ( d->m_widget ) {
        d->m_widget->removeEventFilter( this );
        d->m_widget->update();
    }
}


bool GvPixmapSequenceOverlayPainter::eventFilter( QObject* obj, QEvent* event )
{
    if ( obj == d->m_widget && event->type() == QEvent::Paint ) {
        // make sure we paint after everyone else including other event filters
        obj->removeEventFilter(this); // don't recurse...
        QCoreApplication::sendEvent(obj, event);
        d->paintFrame();
        obj->installEventFilter(this); // catch on...
        return true;
    }

    return false;
}

#include "kpixmapsequenceoverlaypainter.moc"
