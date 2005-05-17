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

#ifndef TSTHREAD_H
#define TSTHREAD_H

#include <qobject.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qdeepcopy.h>
#include <qmutex.h>
#include <private/qucomextra_p.h>

#ifdef TS_QTHREADSTORAGE
#include <qthreadstorage.h>
#else
#include <pthread.h>
#endif

#include "tswaitcondition.h"

// how difficult ...
template< typename T >
T TSDeepCopy( const T& t )
{
    return QDeepCopy< T >( t );
}

class TSCurrentThread;

/**
 Thread class, internally based on QThread, which intentionally doesn't have
 dangerous crap like QThread::terminate() and intentionally has useful features
 like emitting signals to the main thread, currentThread() or support
 for cancelling.
 */
class TSThread
    : public QObject
    {
    Q_OBJECT
    public:
        TSThread();
        virtual ~TSThread();
        /**
         * Starts the thread.
         * @see QThread::start()
         */
        void start();
        /**
         * Waits for the thread to finish.
         * @see QThread::wait()
         */
        void wait( unsigned long time = ULONG_MAX );
        /**
         * Returns true if the thread has finished.
         * @see QThread::finished()
         */
        bool finished() const;
        /**
         * Returns true if the thread is running.
         * @see QThread::running()
         */
        bool running() const;
        /**
         * Sends the thread a request to terminate.
         * The thread must check for cancellation using testCancel()
         * and finish execution (return from run()).
         * @see testCancel()
         */
        void cancel();
        // TODO suspend + resume?
        /**
         * Returns true if a request to terminate is pending.
         * @see cancel()
         */
        bool testCancel() const;
        /**
         * Returns pointer to the current thread, i.e. thread from which
         * this function is called.
         */
        static TSThread* currentThread();
        /**
         * Returns a pointer to the main thread. Mostly useful for currentThread().
         */
        static TSThread* mainThread();
        /**
         * Emits the specified signal in the main thread. This function returns
         * only after all slots connected to the signal have been executed (i.e.
         * it works like normal signal). The signal can have one pointer argument,
         * which can be used for communication in either direction. QObject::sender()
         * in slots is valid.
         * Example:
         * \code
         *   emitSignal( this, SIGNAL( result( int* )), &result_data );
         * \endcode
         * @see postSignal
         * @see emitCancellableSignal
         */
        void emitSignal( QObject* obj, const char* signal );
        template< typename T1 >
        void emitSignal( QObject* obj, const char* signal, const T1& p1 );
        template< typename T1, typename T2 >
        void emitSignal( QObject* obj, const char* signal, const T1& p1, const T2& p2 );
        /**
         * This function works like emitSignal(), but additionally acts as a cancellation
         * point, i.e. calling cancel() on the thread causes premature return.
         * @see emitSignal
         * @see postSignal
         */        
        void emitCancellableSignal( QObject* obj, const char* signal );
        template< typename T1 >
        void emitCancellableSignal( QObject* obj, const char* signal, const T1& p1 );
        template< typename T1, typename T2 >
        void emitCancellableSignal( QObject* obj, const char* signal, const T1& p1, const T2& p2 );
        /**
         * Posts (i.e. it is not executed immediatelly like normal signals)
         * a signal to be emitted in the main thread. The signal cannot
         * have any parameters, use your TSThread derived class instance
         * data members instead. QObject::sender() in slots is valid, unless
         * the thread instance is destroyed before the signal is processed.
         * @see emitSignal
         */
        void postSignal( QObject* obj, const char* signal ); // is emitted _always_ in main thread
    protected:
        /**
         * The code to be executed in the started thread.
         * @see QThread::run()
         */
        virtual void run() = 0;
    signals:
        /**
         * Emitted after the thread is terminated.
         */
        void terminated(); // is emitted _always_ in main thread
    protected:
        /**
         * @internal
         */
        void customEvent( QCustomEvent* e );
    private:
        class SignalEvent
            : public QCustomEvent
            {
            public:
                SignalEvent( const char* sig, QObject* obj, QUObject* o )
                    : QCustomEvent( QEvent::User ), signal( sig ), object( obj ), args( o )
                    {
                    }
                const QCString signal;
                QObject* object;
                QUObject* args;
            };
        class Helper
            : public QThread
            {
            public:
                Helper( TSThread* parent );
            protected:
                virtual void run();
            private:
                TSThread* thread;
            };
        void executeThread();
        static void initCurrentThread();
        bool setCancelData( QMutex*m, QWaitCondition* c );
        void setSignalData( QUObject* o, int i );
        void setSignalData( QUObject* o, const QImage& i );
        void setSignalData( QUObject* o, const QString& s );
        void setSignalData( QUObject* o, bool b );
        void setSignalData( QUObject* o, const QColor& c );
        void setSignalData( QUObject* o, const char* s );
        void setSignalData( QUObject* o, const QSize& );
        void emitSignalInternal( QObject* obj, const char* signal, QUObject* o );
        void emitCancellableSignalInternal( QObject* obj, const char* signal, QUObject* o );
        friend class Helper;
        friend class TSWaitCondition;
        Helper thread;
        bool cancelling;
        bool emit_pending;
        mutable QMutex mutex;
        QMutex signal_mutex;
        TSWaitCondition signal_cond;
        QMutex* cancel_mutex;
        QWaitCondition* cancel_cond;
        bool* deleted_flag;
#ifdef TS_QTHREADSTORAGE
        static QThreadStorage< TSThread** >* current_thread;
#else
        static TSCurrentThread* current_thread;
#endif
        static TSThread* main_thread;
    private:
        TSThread( const TSThread& );
        TSThread& operator=( const TSThread& );
    };

#ifndef TS_QTHREADSTORAGE
/**
 * @internal
 */
class TSCurrentThread
    {
    public:
        TSCurrentThread();
        ~TSCurrentThread();
        TSThread* localData() const;
        void setLocalData( TSThread* t );
    private:
        pthread_key_t key;
    };


inline TSCurrentThread::TSCurrentThread()
    {
    pthread_key_create( &key, NULL );
    }

inline TSCurrentThread::~TSCurrentThread()
    {
    pthread_key_delete( key );
    }

inline void TSCurrentThread::setLocalData( TSThread* t )
    {
    pthread_setspecific( key, t );
    }

inline TSThread* TSCurrentThread::localData() const
    {
    return static_cast< TSThread* >( pthread_getspecific( key ));
    }
#endif

inline
bool TSThread::testCancel() const
    {
    QMutexLocker lock( &mutex );
    return cancelling;
    }

#include <kdebug.h>
inline
bool TSThread::setCancelData( QMutex* m, QWaitCondition* c )
    {
    QMutexLocker lock( &mutex );
    if( cancelling && m != NULL )
        return false;
    cancel_mutex = m;
    cancel_cond = c;
    return true;
    }

inline
TSThread* TSThread::currentThread()
    {
    if( current_thread == NULL )
        initCurrentThread();
#ifdef TS_QTHREADSTORAGE
    return *current_thread->localData();
#else
    return current_thread->localData();
#endif
    }

inline
TSThread* TSThread::mainThread()
    {
    return main_thread;
    }

inline
void TSThread::setSignalData( QUObject* o, int i )
    {
    static_QUType_int.set( o, i );
    }

inline
void TSThread::setSignalData( QUObject* o, const QImage& i )
    {
    static_QUType_varptr.set( o, &i );
    }

inline
void TSThread::setSignalData( QUObject* o, const QString& s )
    {
    static_QUType_QString.set( o, s );
    }

inline
void TSThread::setSignalData( QUObject* o, bool b )
    {
    static_QUType_bool.set( o, b );
    }

inline
void TSThread::setSignalData( QUObject* o, const QColor& c )
    {
    static_QUType_varptr.set( o, &c );
    }

inline
void TSThread::setSignalData( QUObject* o, const char* s )
    {
    static_QUType_charstar.set( o, s );
    }

inline
void TSThread::setSignalData( QUObject* o, const QSize& s )
    {
    static_QUType_varptr.set( o, &s );
    }

inline
void TSThread::emitSignal( QObject* obj, const char* signal )
    {
    QUObject o[ 1 ];
    emitSignalInternal( obj, signal, o );
    }

template< typename T1 >
inline
void TSThread::emitSignal( QObject* obj, const char* signal, const T1& p1 )
    {
    QUObject o[ 2 ];
    setSignalData( o + 1, p1 );
    emitSignalInternal( obj, signal, o );
    }

template< typename T1, typename T2 >
inline
void TSThread::emitSignal( QObject* obj, const char* signal, const T1& p1, const T2& p2 )
    {
    QUObject o[ 3 ];
    setSignalData( o + 1, p1 );
    setSignalData( o + 2, p2 );
    emitSignalInternal( obj, signal, o );
    }

inline
void TSThread::emitCancellableSignal( QObject* obj, const char* signal )
    {
    QUObject o[ 1 ];
    emitCancellableSignalInternal( obj, signal, o );
    }

template< typename T1 >
inline
void TSThread::emitCancellableSignal( QObject* obj, const char* signal, const T1& p1 )
    {
    QUObject o[ 2 ];
    setSignalData( o + 1, p1 );
    emitCancellableSignalInternal( obj, signal, o );
    }

template< typename T1, typename T2 >
inline
void TSThread::emitCancellableSignal( QObject* obj, const char* signal, const T1& p1, const T2& p2 )
    {
    QUObject o[ 3 ];
    setSignalData( o + 1, p1 );
    setSignalData( o + 2, p2 );
    emitCancellableSignalInternal( obj, signal, o );
    }


#endif
