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

#include "tswaitcondition.h"

#include "tsthread.h"

bool TSWaitCondition::wait( QMutex* m, unsigned long time )
    {
    return cond.wait( m, time ); // TODO?
    }

bool TSWaitCondition::cancellableWait( QMutex* m, unsigned long time )
    {
    mutex.lock();
    if( !TSThread::currentThread()->setCancelData( &mutex, &cond ))
        {
        mutex.unlock();
        return false;
        }
    m->unlock();
    bool ret = cond.wait( &mutex, time );
    TSThread::currentThread()->setCancelData( NULL, NULL );
    mutex.unlock();
    m->lock();
    return ret;
    }

void TSWaitCondition::wakeOne()
    {
    QMutexLocker locker( &mutex );
    cond.wakeOne();
    }

void TSWaitCondition::wakeAll()
    {
    QMutexLocker locker( &mutex );
    cond.wakeAll();
    }
