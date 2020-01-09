// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
#ifndef GVDEBUG_H
#define GVDEBUG_H

#include "gwenview_lib_debug.h"

/**
 * Uses this macro if you want your code to abort when the GV_FATAL_FAILS
 * environment variable is set.
 * Most of the time you want to use the various GV_RETURN* macros instead,
 * but this one can be handy in certain situations. For example in a switch
 * case block:
 *
 * switch (state) {
 * case State1:
 *     ...
 *     break;
 * case State2:
 *     ...
 *     break;
 * case State3:
 *     qCWarning(GWENVIEW_LIB_LOG) << "state should not be State3";
 *     GV_FATAL_FAILS;
 *     break;
 * }
 */
#define GV_FATAL_FAILS \
    do { \
        if (!qEnvironmentVariableIsEmpty("GV_FATAL_FAILS")) { \
            qFatal("Aborting because environment variable 'GV_FATAL_FAILS' is set"); \
        } \
    } while (0)

#define GV_RETURN_IF_FAIL(cond) \
    do { \
        if (!(cond)) { \
            qCWarning(GWENVIEW_LIB_LOG) << "Condition '" << #cond << "' failed"; \
            GV_FATAL_FAILS; \
            return; \
        } \
    } while (0)

#define GV_RETURN_VALUE_IF_FAIL(cond, value) \
    do { \
        if (!(cond)) { \
            qCWarning(GWENVIEW_LIB_LOG) << "Condition '" << #cond << "' failed."; \
            GV_FATAL_FAILS; \
            return value; \
        } \
    } while (0)

#define GV_RETURN_IF_FAIL2(cond, msg) \
    do { \
        if (!(cond)) { \
            qCWarning(GWENVIEW_LIB_LOG) << "Condition '" << #cond << "' failed" << msg; \
            GV_FATAL_FAILS; \
            return; \
        } \
    } while (0)

#define GV_RETURN_VALUE_IF_FAIL2(cond, value, msg) \
    do { \
        if (!(cond)) { \
            qCWarning(GWENVIEW_LIB_LOG) << "Condition '" << #cond << "' failed." << msg; \
            GV_FATAL_FAILS; \
            return value; \
        } \
    } while (0)

#define GV_WARN_AND_RETURN(msg) \
    do { \
        qCWarning(GWENVIEW_LIB_LOG) << msg; \
        GV_FATAL_FAILS; \
        return; \
    } while (0)

#define GV_WARN_AND_RETURN_VALUE(value, msg) \
    do { \
        qCWarning(GWENVIEW_LIB_LOG) << msg; \
        GV_FATAL_FAILS; \
        return value; \
    } while (0)

#define GV_LOG(var) qCDebug(GWENVIEW_LIB_LOG) << #var << '=' << (var)

#endif // GVDEBUG_H
