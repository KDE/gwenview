/****************************************************************************

 Copyright (C) 2004 Lubos Lunak        <l.lunak@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

#include "tsthread.h"

#include <qapplication.h>
#include <qmetaobject.h>
#include <kdebug.h>
#include <qguardedptr.h>

#include <assert.h>

#ifdef TS_QTHREADSTORAGE
QThreadStorage< TSThread** >* TSThread::current_thread;
#else
TSCurrentThread* TSThread::current_thread;
#endif
TSThread* TSThread::main_thread = NULL;

class TSMainThread
    : public TSThread
    {
    protected:
        virtual void run() { assert( false ); }
    };

TSThread::Helper::Helper( TSThread* parent )
    : thread( parent )
    {
    assert( parent );
    }

void TSThread::Helper::run()
    {
    thread->executeThread();
    }


TSThread::TSThread()
    : thread( this )
    , cancelling( false )
    , emit_pending( false )
    , cancel_mutex( NULL )
    , cancel_cond( NULL )
    , deleted_flag( NULL )
    {
    }

TSThread::~TSThread()
    {
    if( deleted_flag != NULL )
        *deleted_flag = true;
    }

void TSThread::start()
    {
    if( current_thread == NULL )
        initCurrentThread();
    thread.start();
    }

void TSThread::cancel()
    {
    QMutexLocker lock( &mutex );
    cancelling = true;
    if( cancel_mutex != NULL )
        {
        QMutexLocker lock( cancel_mutex );
        cancel_cond->wakeAll();
        }
    }

void TSThread::wait( unsigned long time )
    {
    thread.wait( time );
    }

bool TSThread::finished() const
    {
    return thread.finished();
    }

bool TSThread::running() const
    {
    return thread.running();
    }

void TSThread::initCurrentThread()
    {
#ifdef TS_QTHREADSTORAGE
    current_thread = new QThreadStorage< TSThread** >();
    typedef TSThread* TSThreadPtr;
    // set pointer for main thread (this must be main thread)
    current_thread->setLocalData( new TSThreadPtr( NULL )); // TODO NULL ?
#else
    current_thread = new TSCurrentThread();
    main_thread = new TSMainThread;
    // set pointer for main thread (this must be main thread)
    current_thread->setLocalData( main_thread );
#endif
    }

void TSThread::executeThread()
    {
#ifdef TS_QTHREADSTORAGE
    // store dynamically allocated pointer, so that
    // QThreadStorage deletes the pointer and not TSThread
    typedef TSThread* TSThreadPtr;
    current_thread->setLocalData( new TSThreadPtr( this ));
#else
    current_thread->setLocalData( this );
#endif
    run();
    postSignal( this, NULL ); // = terminated()
    }

void TSThread::postSignal( QObject* obj, const char* signal )
    {
    assert( currentThread() == this );
    qApp->postEvent( this, new SignalEvent( signal, obj, NULL ));
    }

void TSThread::emitSignalInternal( QObject* obj, const char* signal, QUObject* o )
    {
    assert( currentThread() == this );
    QMutexLocker locker( &signal_mutex );
    emit_pending = true;
    qApp->postEvent( this, new SignalEvent( signal, obj, o ));
    while( emit_pending )
        signal_cond.wait( &signal_mutex );
    }

void TSThread::emitCancellableSignalInternal( QObject* obj, const char* signal, QUObject* o )
    {
    assert( currentThread() == this );
    // can't use this->mutex, because its locking will be triggered by TSWaitCondition
    QMutexLocker locker( &signal_mutex );
    emit_pending = true;
    qApp->postEvent( this, new SignalEvent( signal, obj, o ));
    while( emit_pending && !testCancel())
        signal_cond.cancellableWait( &signal_mutex );
    emit_pending = false; // in case of cancel
    }

void TSThread::customEvent( QCustomEvent* ev )
    {
    SignalEvent* e = static_cast< SignalEvent* >( ev );
    if( e->signal.isEmpty()) // = terminated()
        { // threadExecute() finishes before the actual thread terminates
        if( !finished())
            wait();
        emit terminated();
        return;
        }
    bool deleted = false;
    deleted_flag = &deleted; // this is like QGuardedPtr for self, but faster
    int signal_id = e->object->metaObject()->findSignal( normalizeSignalSlot( e->signal ).data() + 1, true );
    if( signal_id >= 0 )
        e->object->qt_emit( signal_id, e->args );
    else
        kdWarning() << "Cannot emit signal \"" << e->signal << "\"." << endl;
    if( deleted ) // some slot deleted 'this'
        return;
    deleted_flag = NULL;
    QMutexLocker locker( &signal_mutex );
    if( emit_pending )
        {
        emit_pending = false;
        signal_cond.wakeOne();
        }
    }

#include "tsthread.moc"
