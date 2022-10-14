/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef SPLITTER_H
#define SPLITTER_H

// Qt
#include <QSplitter>

namespace Gwenview
{
class SplitterHandle : public QSplitterHandle
{
public:
    SplitterHandle(Qt::Orientation orientation, QSplitter *parent)
        : QSplitterHandle(orientation, parent)
    {
    }
};

/**
 * Home made splitter to be able to define a custom handle which is border with
 * "mid" colored lines.
 */
class Splitter : public QSplitter
{
public:
    Splitter(Qt::Orientation orientation, QWidget *parent)
        : QSplitter(orientation, parent)
    {
    }

protected:
    QSplitterHandle *createHandle() override
    {
        return new SplitterHandle(orientation(), this);
    }
};

} // namespace

#endif /* SPLITTER_H */
