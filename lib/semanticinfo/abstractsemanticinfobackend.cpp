// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "abstractsemanticinfobackend.h"

// Qt
#include <QStringList>
#include <QVariant>

// KDE

// Local

namespace Gwenview
{

TagSet::TagSet()
: QSet<SemanticInfoTag>() {}

TagSet::TagSet(const QSet<SemanticInfoTag>& set)
: QSet<QString>(set) {}

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
TagSet::TagSet(const QList<SemanticInfoTag>& list)
: QSet(list.begin(), list.end())
{
}
#endif

QVariant TagSet::toVariant() const
{
    QStringList lst = values();
    return QVariant(lst);
}

TagSet TagSet::fromVariant(const QVariant& variant)
{
    QStringList lst = variant.toStringList();
    return TagSet::fromList(lst);
}

TagSet TagSet::fromList(const QList<SemanticInfoTag>& list)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return TagSet(list);
#else
    return QSet<SemanticInfoTag>::fromList(lst);
#endif
}

AbstractSemanticInfoBackEnd::AbstractSemanticInfoBackEnd(QObject* parent)
: QObject(parent)
{
    qRegisterMetaType<SemanticInfo>("SemanticInfo");
}

} // namespace
