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

#ifdef TS_QTHREADSTORAGE
#include <qthreadstorage.h>
#else
#include <pthread.h>
#endif

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
         * this function is called. Returns NULL for main thread.
         */
        // TODO don't return NULL for main thread
        static TSThread* currentThread();
    protected:
        /**
         * The code to be executed in the started thread.
         * @see QThread::run()
         */
        virtual void run() = 0;
        /**
         * Posts (i.e. it is not executed immediatelly like normal signals)
         * a signal to be emitted in the main thread. The signal cannot
         * have any parameters, use your TSThread derived class instance
         * data members instead. QObject::sender() is valid, unless the thread
         * instance is destroyed before the signal is processed.
         */
        void postSignal( const char* signal ); // is emitted _always_ in main thread
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
                SignalEvent( const char* sig )
                    : QCustomEvent( QEvent::User ), signal( sig )
                    {
                    }
                const QCString signal;
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
        void setCancelCond( QWaitCondition* c );
        friend class Helper;
        friend class TSWaitCondition;
        Helper thread;
        bool cancelling;
        mutable QMutex mutex;
        QWaitCondition* cancel_cond;
#ifdef TS_QTHREADSTORAGE
        static QThreadStorage< TSThread** >* current_thread;
#else
        static TSCurrentThread* current_thread;
#endif
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

inline
void TSThread::setCancelCond( QWaitCondition* c )
    {
    QMutexLocker lock( &mutex );
    cancel_cond = c;
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

#endif
