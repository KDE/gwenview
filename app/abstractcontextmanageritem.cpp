/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "abstractcontextmanageritem.h"

// Local
#include <lib/contextmanager.h>

namespace Gwenview
{
struct AbstractContextManagerItemPrivate {
    ContextManager *mContextManager = nullptr;
    QWidget *mWidget = nullptr;
};

AbstractContextManagerItem::AbstractContextManagerItem(ContextManager *manager)
    : QObject(manager)
    , d(new AbstractContextManagerItemPrivate)
{
    d->mContextManager = manager;
    d->mWidget = nullptr;
}

AbstractContextManagerItem::~AbstractContextManagerItem()
{
    delete d;
}

ContextManager *AbstractContextManagerItem::contextManager() const
{
    return d->mContextManager;
}

QWidget *AbstractContextManagerItem::widget() const
{
    return d->mWidget;
}

void AbstractContextManagerItem::setWidget(QWidget *widget)
{
    d->mWidget = widget;
}

} // namespace

#include "moc_abstractcontextmanageritem.cpp"
