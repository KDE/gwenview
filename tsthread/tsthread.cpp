#include "tsthread.h"

#include <qapplication.h>
#include <qmetaobject.h>
#include <kdebug.h>

#include <assert.h>

#ifdef TS_QTHREADSTORAGE
QThreadStorage< TSThread** >* TSThread::current_thread;
#else
TSCurrentThread* TSThread::current_thread;
#endif

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
#ifdef TS_QTHREADSTORAGE
    current_thread = new QThreadStorage< TSThread** >();
    typedef TSThread* TSThreadPtr;
    // set pointer for main thread (this must be main thread)
    current_thread->setLocalData( new TSThreadPtr( NULL )); // TODO NULL ?
#else
    current_thread = new TSCurrentThread();
    // set pointer for main thread (this must be main thread)
    current_thread->setLocalData( NULL ); // TODO NULL ?
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
    else
        kdWarning() << "Cannot emit signal \"" << signal << "\"." << endl;
    }

#include "tsthread.moc"
