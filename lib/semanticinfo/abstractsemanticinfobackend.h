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
#ifndef ABSTRACTSEMANTICINFOBACKEND_H
#define ABSTRACTSEMANTICINFOBACKEND_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>
#include <QSet>

// KF

// Local

class QUrl;

namespace Gwenview
{
using SemanticInfoTag = QString;

/**
 * This class represents the set of tags associated to an url.
 *
 * It provides convenience methods to convert to and from QVariant, which are
 * useful to communicate with SemanticInfoDirModel.
 */
class GWENVIEWLIB_EXPORT TagSet : public QSet<SemanticInfoTag>
{
public:
    TagSet();
    TagSet(const QSet<SemanticInfoTag> &);

    QVariant toVariant() const;
    static TagSet fromVariant(const QVariant &);
    static TagSet fromList(const QList<SemanticInfoTag> &);

private:
    TagSet(const QList<SemanticInfoTag> &);
};

/**
 * A POD struct used by AbstractSemanticInfoBackEnd to store the metadata
 * associated to an url.
 */
struct SemanticInfo {
    int mRating;
    QString mDescription;
    TagSet mTags;
};

/**
 * An abstract class, used by SemanticInfoDirModel to store and retrieve metadata.
 */
class AbstractSemanticInfoBackEnd : public QObject
{
    Q_OBJECT
public:
    explicit AbstractSemanticInfoBackEnd(QObject *parent);

    virtual TagSet allTags() const = 0;

    virtual void refreshAllTags() = 0;

    virtual void storeSemanticInfo(const QUrl &, const SemanticInfo &) = 0;

    virtual void retrieveSemanticInfo(const QUrl &) = 0;

    virtual QString labelForTag(const SemanticInfoTag &) const = 0;

    /**
     * Return a tag for a label. Will emit tagAdded() if the tag had to be
     * created.
     */
    virtual SemanticInfoTag tagForLabel(const QString &) = 0;

Q_SIGNALS:
    void semanticInfoRetrieved(const QUrl &, const SemanticInfo &);

    /**
     * Emitted whenever a new tag is added to allTags()
     */
    void tagAdded(const SemanticInfoTag &, const QString &label);
};

} // namespace

#endif /* ABSTRACTSEMANTICINFOBACKEND_H */
