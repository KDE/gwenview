#ifndef GVTHREAD_H
#define GVTHREAD_H

#include <qobject.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qthreadstorage.h>
#include <qdeepcopy.h>
#include <qmutex.h>

// how difficult ...
template< typename T >
T GVDeepCopy( const T& t )
{
    return QDeepCopy< T >( t );
}

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
        friend class GVCancellable;
        Helper thread;
        bool cancelling;
        mutable QMutex mutex;
        QWaitCondition* cancel_cond;
        static QThreadStorage< TSThread** >* current_thread;
    };

/**
 * Class allowing interruption of waiting on QWaitCondition when TSThread::cancel()
 * is called. Example:
 * \code
 * void MyThread::run()
 *     {
 *     QMutexLocker lock( &mutex );
 *     while( !testCancel())
 *         {
 *         while( !dataReady )
 *             {
 *             GVCancellable c( &condition );
 *             condition.wait( &mutex );
 *             if( testCancel())
 *                 return;
 *             }
 *         processData();
 *         }
 *     }
 * \endcode
 */
class GVCancellable
    {
    public:
        GVCancellable( QWaitCondition* c );
        ~GVCancellable();
    private:
        QWaitCondition* cond;
    };

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
    return *current_thread->localData();
    }

inline
GVCancellable::GVCancellable( QWaitCondition* c )
    : cond( c )
    {
    TSThread::currentThread()->setCancelCond( c );
    }


inline
GVCancellable::~GVCancellable()
    {
    TSThread::currentThread()->setCancelCond( NULL );
    }

#endif
