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

#ifndef TSWAITCONDITION_H
#define TSWAITCONDITION_H

#include <qmutex.h>
#include <qwaitcondition.h>

class TSWaitCondition
    {
    public:
        TSWaitCondition();
        ~TSWaitCondition();
        bool wait( QMutex* mutex, unsigned long time = ULONG_MAX );
        bool wait( QMutex& mutex, unsigned long time = ULONG_MAX );
        bool cancellableWait( QMutex* mutex, unsigned long time = ULONG_MAX );
        bool cancellableWait( QMutex& mutex, unsigned long time = ULONG_MAX );
        void wakeOne();
        void wakeAll();
    private:
        QMutex mutex;
        QWaitCondition cond;
    private:
        TSWaitCondition( const TSWaitCondition& );
        TSWaitCondition& operator=( const TSWaitCondition& );
    };

inline
TSWaitCondition::TSWaitCondition()
    {
    }

inline
TSWaitCondition::~TSWaitCondition()
    {
    }

inline
bool TSWaitCondition::wait( QMutex& mutex, unsigned long time )
    {
    return wait( &mutex, time );
    }

inline
bool TSWaitCondition::cancellableWait( QMutex& mutex, unsigned long time )
    {
    return cancellableWait( &mutex, time );
    }

#endif
