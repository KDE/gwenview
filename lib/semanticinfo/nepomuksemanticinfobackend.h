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
#ifndef NEPOMUKSEMANTICINFOBACKEND_H
#define NEPOMUKSEMANTICINFOBACKEND_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include "abstractsemanticinfobackend.h"

namespace Gwenview
{

struct NepomukSemanticInfoBackEndPrivate;

/**
 * A real metadata backend using Nepomuk to store and retrieve metadata.
 */
class GWENVIEWLIB_EXPORT NepomukSemanticInfoBackEnd : public AbstractSemanticInfoBackEnd
{
    Q_OBJECT
public:
    NepomukSemanticInfoBackEnd(QObject* parent);
    ~NepomukSemanticInfoBackEnd();

    virtual TagSet allTags() const;

    virtual void refreshAllTags();

    virtual void storeSemanticInfo(const KUrl&, const SemanticInfo&);

    virtual void retrieveSemanticInfo(const KUrl&);

    virtual QString labelForTag(const SemanticInfoTag&) const;

    virtual SemanticInfoTag tagForLabel(const QString&);

    void emitSemanticInfoRetrieved(const KUrl&, const SemanticInfo&);

private:
    NepomukSemanticInfoBackEndPrivate* const d;
};

} // namespace

#endif /* NEPOMUKSEMANTICINFOBACKEND_H */
