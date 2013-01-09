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
#include "semanticinfobackendtest.moc"

// Qt
#include <QSignalSpy>

// KDE
#include <KDebug>
#include <KRandom>
#include <KTemporaryFile>
#include <qtest_kde.h>

// Local
#include "testutils.h"
#include <config-gwenview.h>

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
#include <lib/semanticinfo/fakesemanticinfobackend.h>

#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
#include <Nepomuk2/ResourceManager>
#include <lib/semanticinfo/nepomuksemanticinfobackend.h>

#else
#ifdef __GNUC__
#error No metadata backend defined
#endif
#endif

QTEST_KDEMAIN(Gwenview::SemanticInfoBackEndTest, GUI)

namespace Gwenview
{

SemanticInfoBackEndClient::SemanticInfoBackEndClient(AbstractSemanticInfoBackEnd* backEnd)
: mBackEnd(backEnd)
{
    connect(backEnd, SIGNAL(semanticInfoRetrieved(KUrl,SemanticInfo)),
            SLOT(slotSemanticInfoRetrieved(KUrl,SemanticInfo)));
}

void SemanticInfoBackEndClient::slotSemanticInfoRetrieved(const KUrl& url, const SemanticInfo& semanticInfo)
{
    mSemanticInfoForUrl[url] = semanticInfo;
}

void SemanticInfoBackEndTest::initTestCase()
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK
    if (Nepomuk2::ResourceManager::instance()->init() != 0) {
        QSKIP("This test needs Nepomuk", SkipAll);
    }
#endif
    qRegisterMetaType<KUrl>("KUrl");
    qRegisterMetaType<QString>("SemanticInfoTag");
}

void SemanticInfoBackEndTest::init()
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
    mBackEnd = new FakeSemanticInfoBackEnd(0, FakeSemanticInfoBackEnd::InitializeEmpty);
#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
    mBackEnd = new NepomukSemanticInfoBackEnd(0);
#endif
}

void SemanticInfoBackEndTest::cleanup()
{
    delete mBackEnd;
    mBackEnd = 0;
}

/**
 * Get and set the rating of a temp file
 */
void SemanticInfoBackEndTest::testRating()
{
    KTemporaryFile temp;
    temp.setSuffix(".metadatabackendtest");
    QVERIFY(temp.open());

    KUrl url;
    url.setPath(temp.fileName());

    SemanticInfoBackEndClient client(mBackEnd);
    QSignalSpy spy(mBackEnd, SIGNAL(semanticInfoRetrieved(KUrl,SemanticInfo)));
    mBackEnd->retrieveSemanticInfo(url);
    QVERIFY(waitForSignal(spy));

    SemanticInfo semanticInfo = client.semanticInfoForUrl(url);
    QCOMPARE(semanticInfo.mRating, 0);

    semanticInfo.mRating = 5;
    mBackEnd->storeSemanticInfo(url, semanticInfo);
}

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

} // namespace
