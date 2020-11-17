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
// Local
#include "semanticinfobackendtest.h"

// Qt
#include <QSignalSpy>

// KDE
#include <QDebug>
#include <KRandom>
#include <QTemporaryFile>
#include <QTest>

// Local
#include "testutils.h"
#include <config-gwenview.h>

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
#include <lib/semanticinfo/fakesemanticinfobackend.h>

#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
#include <lib/semanticinfo/baloosemanticinfobackend.h>

#else
#ifdef __GNUC__
#error No metadata backend defined
#endif
#endif

QTEST_MAIN(Gwenview::SemanticInfoBackEndTest)

namespace Gwenview
{

SemanticInfoBackEndClient::SemanticInfoBackEndClient(AbstractSemanticInfoBackEnd* backEnd)
: mBackEnd(backEnd)
{
    connect(backEnd, SIGNAL(semanticInfoRetrieved(QUrl,SemanticInfo)),
            SLOT(slotSemanticInfoRetrieved(QUrl,SemanticInfo)));
}

void SemanticInfoBackEndClient::slotSemanticInfoRetrieved(const QUrl &url, const SemanticInfo& semanticInfo)
{
    mSemanticInfoForUrl[url] = semanticInfo;
}

void SemanticInfoBackEndTest::initTestCase()
{
    qRegisterMetaType<QUrl>("QUrl");
    qRegisterMetaType<QString>("SemanticInfoTag");
}

void SemanticInfoBackEndTest::init()
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
    mBackEnd = new FakeSemanticInfoBackEnd(nullptr, FakeSemanticInfoBackEnd::InitializeEmpty);
#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    mBackEnd = new BalooSemanticInfoBackend(nullptr);
#endif
}

void SemanticInfoBackEndTest::cleanup()
{
    delete mBackEnd;
    mBackEnd = nullptr;
}

/**
 * Get and set the rating of a temp file
 */
void SemanticInfoBackEndTest::testRating()
{
    QTemporaryFile temp("XXXXXX.metadatabackendtest");
    QVERIFY(temp.open());

    QUrl url;
    url.setPath(temp.fileName());

    SemanticInfoBackEndClient client(mBackEnd);
    QSignalSpy spy(mBackEnd, SIGNAL(semanticInfoRetrieved(QUrl,SemanticInfo)));
    mBackEnd->retrieveSemanticInfo(url);
    QVERIFY(waitForSignal(spy));

    SemanticInfo semanticInfo = client.semanticInfoForUrl(url);
    QCOMPARE(semanticInfo.mRating, 0);

    semanticInfo.mRating = 5;
    mBackEnd->storeSemanticInfo(url, semanticInfo);
}

#if 0
// Disabled because Baloo does not work like Nepomuk: it does not create tags
// independently of files.
void SemanticInfoBackEndTest::testTagForLabel()
{
    QSignalSpy spy(mBackEnd, SIGNAL(tagAdded(SemanticInfoTag,QString)));

    TagSet oldAllTags = mBackEnd->allTags();
    QString label = "testTagForLabel-" + KRandom::randomString(5);
    SemanticInfoTag tag1 = mBackEnd->tagForLabel(label);
    QVERIFY(!tag1.isEmpty());
    QVERIFY(!oldAllTags.contains(tag1));
    QVERIFY(mBackEnd->allTags().contains(tag1));

    // This is a new tag, we should receive a signal
    QCOMPARE(spy.count(), 1);

    SemanticInfoTag tag2 = mBackEnd->tagForLabel(label);
    QCOMPARE(tag1, tag2);
    // This is not a new tag, we should not receive a signal
    QCOMPARE(spy.count(), 1);

    QString label2 = mBackEnd->labelForTag(tag2);
    QCOMPARE(label, label2);
}
#endif

} // namespace
