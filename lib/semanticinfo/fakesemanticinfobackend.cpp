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
#include "fakesemanticinfobackend.h"

// Qt
#include <QStringList>

// KDE
#include <QUrl>

// Local

namespace Gwenview
{

FakeSemanticInfoBackEnd::FakeSemanticInfoBackEnd(QObject* parent, InitializeMode mode)
: AbstractSemanticInfoBackEnd(parent)
, mInitializeMode(mode)
{
    mAllTags
            << tagForLabel("beach")
            << tagForLabel("mountains")
            << tagForLabel("wallpaper")
            ;
}

void FakeSemanticInfoBackEnd::storeSemanticInfo(const QUrl &url, const SemanticInfo& semanticInfo)
{
    mSemanticInfoForUrl[url] = semanticInfo;
    mergeTagsWithAllTags(semanticInfo.mTags);
}

void FakeSemanticInfoBackEnd::mergeTagsWithAllTags(const TagSet& set)
{
    int size = mAllTags.size();
    mAllTags |= set;
    if (mAllTags.size() > size) {
        //emit allTagsUpdated();
    }
}

TagSet FakeSemanticInfoBackEnd::allTags() const
{
    return mAllTags;
}

void FakeSemanticInfoBackEnd::refreshAllTags()
{
}

void FakeSemanticInfoBackEnd::retrieveSemanticInfo(const QUrl &url)
{
    if (!mSemanticInfoForUrl.contains(url)) {
        QString urlString = url.url();
        SemanticInfo semanticInfo;
        if (mInitializeMode == InitializeRandom) {
            semanticInfo.mRating = int(urlString.length()) % 6;
            semanticInfo.mDescription = url.fileName();
            const QStringList lst = url.path().split('/');
            for (const QString & token : lst) {
                if (!token.isEmpty()) {
                    semanticInfo.mTags << '#' + token.toLower();
                }
            }
            semanticInfo.mTags << QString("#length-%1").arg(url.fileName().length());

            mergeTagsWithAllTags(semanticInfo.mTags);
        } else {
            semanticInfo.mRating = 0;
        }
        mSemanticInfoForUrl[url] = semanticInfo;
    }
    emit semanticInfoRetrieved(url, mSemanticInfoForUrl.value(url));
}

QString FakeSemanticInfoBackEnd::labelForTag(const SemanticInfoTag& tag) const
{
    return tag[1].toUpper() + tag.mid(2);
}

SemanticInfoTag FakeSemanticInfoBackEnd::tagForLabel(const QString& label)
{
    return '#' + label.toLower();
}

} // namespace
