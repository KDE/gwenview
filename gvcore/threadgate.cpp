// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau
 
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

#include "threadgate.moc"

#include "tsthread/tsthread.h"

namespace Gwenview {

// The trick is simple. This object connects its slot to its signal, then
// emits the signal using emitSignal(), and the slot gets called in the main
// thread. In the main thread the slot does everything that should be done
// in the main thread, and returns the data using the signal/slot reference
// arguments. As the thread is blocked waiting on the signal to finish,
// there's even no need to do any locking.

ThreadGate::ThreadGate() {
	connect( this, SIGNAL( signalColor( QColor&, const char* )),
		this, SLOT( slotColor( QColor&, const char* )));
}

ThreadGate* ThreadGate::instance() {
	static ThreadGate gate;
	return &gate;
}

QColor ThreadGate::color( const char* name ) {
	if( name == NULL || name[ 0 ] == '\0' || name[ 0 ] == '#' )
		return QColor( name );
	// named color ... needs to be created in the main thread
	if( TSThread::currentThread() == TSThread::mainThread())
		return QColor( name );
	QColor col;
	TSThread::currentThread()->emitCancellableSignal( this, SIGNAL( signalColor( QColor&, const char* )), col, name );
	return col;
}

void ThreadGate::slotColor( QColor& col, const char* name ) {
	col = QColor( name );
}

} // namespace
