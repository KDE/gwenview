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
#ifndef FAKESEMANTICINFOBACKEND_H
#define FAKESEMANTICINFOBACKEND_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QHash>
#include <QUrl>

// KF

// Local
#include "abstractsemanticinfobackend.h"

namespace Gwenview
{
/**
 * A fake metadata backend, useful to test the ui layer.
 * It provides fake rating values based on the image url.
 */
class GWENVIEWLIB_EXPORT FakeSemanticInfoBackEnd : public AbstractSemanticInfoBackEnd
{
    Q_OBJECT
public:
    enum InitializeMode {
        InitializeEmpty,
        InitializeRandom,
    };
    FakeSemanticInfoBackEnd(QObject *parent, InitializeMode initializeMode);

    TagSet allTags() const override;

    void refreshAllTags() override;

    void storeSemanticInfo(const QUrl &, const SemanticInfo &) override;

    void retrieveSemanticInfo(const QUrl &) override;

    QString labelForTag(const SemanticInfoTag &) const override;

    SemanticInfoTag tagForLabel(const QString &) override;

private:
    void mergeTagsWithAllTags(const TagSet &);

    QHash<QUrl, SemanticInfo> mSemanticInfoForUrl;
    InitializeMode mInitializeMode;
    TagSet mAllTags;
};

} // namespace

#endif /* FAKESEMANTICINFOBACKEND_H */
