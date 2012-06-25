// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>
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
#elif defined(Q_OS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#endif

namespace Gwenview
{

namespace MemoryUtils
{

// This code has been copied from okular/core/document.cpp
qulonglong getTotalMemory()
{
    static qulonglong cachedValue = 0;
    if ( cachedValue )
        return cachedValue;

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
        return (cachedValue = 134217728);

    QTextStream readStream( &memFile );
    while ( true )
    {
        QString entry = readStream.readLine();
        if ( entry.isNull() ) break;
        if ( entry.startsWith( "MemTotal:" ) )
            return (cachedValue = (Q_UINT64_C(1024) * entry.section( ' ', -2, -2 ).toULongLong()));
    }
#elif defined(Q_OS_FREEBSD)
    qulonglong physmem;
    int mib[] = {CTL_HW, HW_PHYSMEM};
    size_t len = sizeof( physmem );
    if ( sysctl( mib, 2, &physmem, &len, NULL, 0 ) == 0 )
        return (cachedValue = physmem);
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    GlobalMemoryStatusEx (&stat);

    return ( cachedValue = stat.ullTotalPhys );
#endif
    return (cachedValue = 134217728);
}

qulonglong getFreeMemory()
{
    static QTime lastUpdate = QTime::currentTime().addSecs(-3);
    static qulonglong cachedValue = 0;

    if ( qAbs( lastUpdate.secsTo( QTime::currentTime() ) ) <= 2 )
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
    static const int nElems = 5;
    QString names[nElems] = { "MemFree:", "Buffers:", "Cached:", "SwapFree:", "SwapTotal:" };
    qulonglong values[nElems] = { 0, 0, 0, 0, 0 };
    bool foundValues[nElems] = { false, false, false, false, false };
    while ( true )
    {
        entry = readStream.readLine();
        if ( entry.isNull() ) break;
        for ( int i = 0; i < nElems; ++i )
        {
            if ( entry.startsWith( names[i] ) )
            {
                values[i] = entry.section( ' ', -2, -2 ).toULongLong( &foundValues[i] );
            }
        }
    }
    memFile.close();
    bool found = true;
    for ( int i = 0; found && i < nElems; ++i )
        found = found && foundValues[i];
    if ( found )
    {
        memoryFree = values[0] + values[1] + values[2] + values[3];
        if ( values[4] > memoryFree )
            memoryFree = 0;
        else
            memoryFree -= values[4];
    }

    lastUpdate = QTime::currentTime();

    return ( cachedValue = (Q_UINT64_C(1024) * memoryFree) );
#elif defined(Q_OS_FREEBSD)
    qulonglong cache, inact, free, psize;
    size_t cachelen, inactlen, freelen, psizelen;
    cachelen = sizeof( cache );
    inactlen = sizeof( inact );
    freelen = sizeof( free );
    psizelen = sizeof( psize );
    // sum up inactive, cached and free memory
    if ( sysctlbyname( "vm.stats.vm.v_cache_count", &cache, &cachelen, NULL, 0 ) == 0 &&
            sysctlbyname( "vm.stats.vm.v_inactive_count", &inact, &inactlen, NULL, 0 ) == 0 &&
            sysctlbyname( "vm.stats.vm.v_free_count", &free, &freelen, NULL, 0 ) == 0 &&
            sysctlbyname( "vm.stats.vm.v_page_size", &psize, &psizelen, NULL, 0 ) == 0 )
    {
        lastUpdate = QTime::currentTime();
        return (cachedValue = (cache + inact + free) * psize);
    }
    else
    {
        return 0;
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
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
