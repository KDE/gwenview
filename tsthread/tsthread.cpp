#include "tsthread.h"

#include <qapplication.h>
#include <qmetaobject.h>

#include <assert.h>

QThreadStorage< TSThread** >* TSThread::current_thread;

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
    : thread( this ), cancelling( false ), cancel_cond( NULL )
    {
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
    if( cancel_cond != NULL )
        cancel_cond->wakeAll();
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
    current_thread = new QThreadStorage< TSThread** >();
    typedef TSThread* TSThreadPtr;
    // set pointer for main thread (this must be main thread)
    current_thread->setLocalData( new TSThreadPtr( NULL )); // TODO NULL ?
    }

void TSThread::executeThread()
    {
    // store dynamically allocated pointer, so that
    // QThreadStorage deletes the pointer and not TSThread
    typedef TSThread* TSThreadPtr;
    current_thread->setLocalData( new TSThreadPtr( this ));
    run();
    postSignal( NULL ); // = terminated()
    }

void TSThread::postSignal( const char* signal )
    {
    qApp->postEvent( this, new SignalEvent( signal ));
    }

void TSThread::customEvent( QCustomEvent* e )
    {
    QCString signal = static_cast< SignalEvent* >( e )->signal;
    if( signal.isEmpty()) // = terminated()
        { // threadExecute() finishes before the actual thread terminates
        if( !finished())
            wait();
        emit terminated();
        return;
        }
    int signal_id = metaObject()->findSignal( normalizeSignalSlot( signal ).data() + 1, true );
    if( signal_id >= 0 )
        qt_emit( signal_id, NULL );
    }

#include "tsthread.moc"
