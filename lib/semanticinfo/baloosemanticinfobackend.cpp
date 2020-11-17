// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>
Copyright 2014 Vishesh Handa <me@vhanda.in>

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
#include "baloosemanticinfobackend.h"

// Local
#include <lib/gvdebug.h>

// Qt
#include "gwenview_lib_debug.h"
#include <QUrl>

// KDE

// Baloo
#include <Baloo/TagListJob>
#include <KFileMetaData/UserMetaData>

namespace Gwenview
{

struct BalooSemanticInfoBackend::Private
{
    TagSet mAllTags;
};

BalooSemanticInfoBackend::BalooSemanticInfoBackend(QObject* parent)
: AbstractSemanticInfoBackEnd(parent)
, d(new BalooSemanticInfoBackend::Private)
{
}

BalooSemanticInfoBackend::~BalooSemanticInfoBackend()
{
    delete d;
}

TagSet BalooSemanticInfoBackend::allTags() const
{
    if (d->mAllTags.isEmpty()) {
        const_cast<BalooSemanticInfoBackend*>(this)->refreshAllTags();
    }
    return d->mAllTags;
}

void BalooSemanticInfoBackend::refreshAllTags()
{
    Baloo::TagListJob* job = new Baloo::TagListJob();
    job->exec();

    d->mAllTags.clear();
    const QStringList tags = job->tags();
    for (const QString& tag : tags) {
        d->mAllTags << tag;
    }
}

void BalooSemanticInfoBackend::storeSemanticInfo(const QUrl &url, const SemanticInfo& semanticInfo)
{
    KFileMetaData::UserMetaData md(url.toLocalFile());
    md.setRating(semanticInfo.mRating);
    md.setUserComment(semanticInfo.mDescription);
    md.setTags(semanticInfo.mTags.values());
}

void BalooSemanticInfoBackend::retrieveSemanticInfo(const QUrl &url)
{
    KFileMetaData::UserMetaData md(url.toLocalFile());

    SemanticInfo si;
    si.mRating = md.rating();
    si.mDescription = md.userComment();
    si.mTags = TagSet::fromList(md.tags());

    emit semanticInfoRetrieved(url, si);
}

QString BalooSemanticInfoBackend::labelForTag(const SemanticInfoTag& uriString) const
{
    return uriString;
}

SemanticInfoTag BalooSemanticInfoBackend::tagForLabel(const QString& label)
{
    return label;
}

} // namespace
