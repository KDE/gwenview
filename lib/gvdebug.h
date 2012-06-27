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

#include <KDebug>

#define GV_RETURN_IF_FAIL(cond) \
    if (!(cond)) { \
        kWarning() << "Condition '" << #cond << "' failed"; \
        return; \
    }

#define GV_RETURN_VALUE_IF_FAIL(cond, value) \
    if (!(cond)) { \
        kWarning() << "Condition '" << #cond << "' failed."; \
        return value; \
    }

#define GV_RETURN_IF_FAIL2(cond, msg) \
    if (!(cond)) { \
        kWarning() << "Condition '" << #cond << "' failed" << msg; \
        return; \
    }

#define GV_RETURN_VALUE_IF_FAIL2(cond, value, msg) \
    if (!(cond)) { \
        kWarning() << "Condition '" << #cond << "' failed." << msg; \
        return value; \
    }

#define GV_LOG(var) kDebug() << #var << '=' << (var)

#endif // GVDEBUG_H