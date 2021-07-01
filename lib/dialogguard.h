/*
Gwenview: an image viewer
Copyright 2018 Anthony Fieroni <bvbfan@abv.bg>

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
#ifndef DIALOGGUARD_H
#define DIALOGGUARD_H

// std
#include <utility>

// Qt
#include <QPointer>

template<class T>
class DialogGuard
{
    QPointer<T> m_dialog;

public:
    template<class... Args>
    DialogGuard(Args &&...args)
    {
        m_dialog = new T(std::forward<Args>(args)...);
    }

    ~DialogGuard()
    {
        delete m_dialog;
    }

    T *data() const
    {
        return m_dialog.data();
    }

    T *operator->() const
    {
        return m_dialog.data();
    }

    operator bool() const
    {
        return !m_dialog.isNull();
    }
};

#endif // DIALOGGUARD_H
