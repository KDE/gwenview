/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "metadatabackendtest.moc"

// Qt
#include <QSignalSpy>

// KDE
#include <kdebug.h>
#include <krandom.h>
#include <ktemporaryfile.h>
#include <qtest_kde.h>

// Local
#include "testutils.h"
#include <config-gwenview.h>

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
#include <lib/semanticinfo/fakesemanticinfobackend.h>

#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
#include <lib/semanticinfo/nepomuksemanticinfobackend.h>

#else
#ifdef __GNUC__
	#error No metadata backend defined
#endif
#endif

QTEST_KDEMAIN( Gwenview::MetaDataBackEndTest, GUI )

namespace Gwenview {


MetaDataBackEndClient::MetaDataBackEndClient(AbstractMetaDataBackEnd* backEnd)
: mBackEnd(backEnd) {
	connect(backEnd, SIGNAL(metaDataRetrieved(const KUrl&, const MetaData&)),
		SLOT(slotMetaDataRetrieved(const KUrl&, const MetaData&)) );
}


void MetaDataBackEndClient::slotMetaDataRetrieved(const KUrl& url, const MetaData& metaData) {
	mMetaDataForUrl[url] = metaData;
}


void MetaDataBackEndTest::initTestCase() {
	qRegisterMetaType<KUrl>("KUrl");
}


void MetaDataBackEndTest::init() {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
	mBackEnd.reset(new FakeMetaDataBackEnd(0, FakeMetaDataBackEnd::InitializeEmpty));
#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
	mBackEnd.reset(new NepomukMetaDataBackEnd(0));
#endif
}


/**
 * Get and set the rating of a temp file
 */
void MetaDataBackEndTest::testRating() {
	KTemporaryFile temp;
	temp.setSuffix(".metadatabackendtest");
	QVERIFY(temp.open());

	KUrl url;
	url.setPath(temp.fileName());

	MetaDataBackEndClient client(mBackEnd.get());
	QSignalSpy spy(mBackEnd.get(), SIGNAL(metaDataRetrieved(const KUrl&, const MetaData&)) );
	mBackEnd->retrieveMetaData(url);
	QVERIFY(waitForSignal(spy));

	MetaData metaData = client.metaDataForUrl(url);
	QCOMPARE(metaData.mRating, 0);

	metaData.mRating = 5;
	mBackEnd->storeMetaData(url, metaData);
}


void MetaDataBackEndTest::testTagForLabel() {
	QString label = "testTagForLabel-" + KRandom::randomString(5);
	MetaDataTag tag1 = mBackEnd->tagForLabel(label);
	QVERIFY(!tag1.isEmpty());

	MetaDataTag tag2 = mBackEnd->tagForLabel(label);
	QCOMPARE(tag1, tag2);

	QString label2 = mBackEnd->labelForTag(tag2);
	QCOMPARE(label, label2);
}


} // namespace
