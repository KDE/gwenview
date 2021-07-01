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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef SEMANTICINFOBACKENDTEST_H
#define SEMANTICINFOBACKENDTEST_H

// Qt
#include <QHash>
#include <QObject>
#include <QUrl>

// KF

// Local
#include <lib/semanticinfo/abstractsemanticinfobackend.h>

namespace Gwenview
{
/**
 * Helper class which gathers the metadata retrieved when
 * AbstractSemanticInfoBackEnd::retrieveSemanticInfo() is called.
 */
class SemanticInfoBackEndClient : public QObject
{
    Q_OBJECT
public:
    SemanticInfoBackEndClient(AbstractSemanticInfoBackEnd *);

    SemanticInfo semanticInfoForUrl(const QUrl &url) const
    {
        return mSemanticInfoForUrl.value(url);
    }

private Q_SLOTS:
    void slotSemanticInfoRetrieved(const QUrl &, const SemanticInfo &);

private:
    QHash<QUrl, SemanticInfo> mSemanticInfoForUrl;
    AbstractSemanticInfoBackEnd *mBackEnd;
};

class SemanticInfoBackEndTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void testRating();
#if 0
    void testTagForLabel();
#endif

private:
    AbstractSemanticInfoBackEnd *mBackEnd;
};

} // namespace

#endif // SEMANTICINFOBACKENDTEST_H
