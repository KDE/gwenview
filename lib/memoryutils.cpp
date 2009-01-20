// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>
Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>
Copyright (C) 2004-2007 by Albert Astals Cid <aacid@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "memoryutils.h"

// Qt
#include <QFile>
#include <QTextStream>
#include <QTime>

// System
#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

namespace Gwenview {


namespace MemoryUtils {

// This code has been copied from okular/core/document.cpp
qulonglong getTotalMemory() {
    static qulonglong cachedValue = 0;
    if ( cachedValue )
        return cachedValue;

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
        return (cachedValue = 134217728);

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    QTextStream readStream( &memFile );
     while ( true )
    {
        QString entry = readStream.readLine();
        if ( entry.isNull() ) break;
        if ( entry.startsWith( "MemTotal:" ) )
            return (cachedValue = (Q_UINT64_C(1024) * entry.section( ' ', -2, -2 ).toULongLong()));
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;

    GlobalMemoryStatusEx (&stat);

    return ( cachedValue = stat.ullTotalPhys );
#endif
    return (cachedValue = 134217728);
}


qulonglong getFreeMemory() {
    static QTime lastUpdate = QTime::currentTime();
    static qulonglong cachedValue = 0;

    if ( lastUpdate.secsTo( QTime::currentTime() ) <= 2 )
        return cachedValue;

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return MEMORY FULL
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
        return 0;

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    qulonglong memoryFree = 0;
    QString entry;
    QTextStream readStream( &memFile );
    while ( true )
    {
        entry = readStream.readLine();
        if ( entry.isNull() ) break;
        if ( entry.startsWith( "MemFree:" ) ||
                entry.startsWith( "Buffers:" ) ||
                entry.startsWith( "Cached:" ) ||
                entry.startsWith( "SwapFree:" ) )
            memoryFree += entry.section( ' ', -2, -2 ).toULongLong();
        if ( entry.startsWith( "SwapTotal:" ) )
            memoryFree -= entry.section( ' ', -2, -2 ).toULongLong();
    }
    memFile.close();

    lastUpdate = QTime::currentTime();

    return ( cachedValue = (Q_UINT64_C(1024) * memoryFree) );
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;

    GlobalMemoryStatusEx (&stat);

    lastUpdate = QTime::currentTime();

    return ( cachedValue = stat.ullAvailPhys );
#else
    // tell the memory is full.. will act as in LOW profile
    return 0;
#endif
}

} // MemoryUtils namespace

} // Gwenview namespace
