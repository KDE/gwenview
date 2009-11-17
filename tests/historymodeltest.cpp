/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "historymodeltest.moc"

// Qt

// KDE
#include <kdebug.h>
#include <kfileplacesmodel.h>
#include <ktempdir.h>
#include <qtest_kde.h>

// Local
#include "../lib/historymodel.h"

QTEST_KDEMAIN(HistoryModelTest, GUI)

using namespace Gwenview;

void testModel(const HistoryModel& model, const KUrl& u1, const KUrl& u2) {
	QModelIndex index;
	KUrl url;
	QCOMPARE(model.rowCount(), 2);

	index = model.index(0, 0);
	QVERIFY(index.isValid());
	url = model.data(index, KFilePlacesModel::UrlRole).value<QUrl>();
	QCOMPARE(url, u1);

	index = model.index(1, 0);
	QVERIFY(index.isValid());
	url = model.data(index, KFilePlacesModel::UrlRole).value<QUrl>();
	QCOMPARE(url, u2);

}

void HistoryModelTest::testAddUrl() {
	KUrl u1 = KUrl::fromPath("/home");
	KUrl u2 = KUrl::fromPath("/root");
	KTempDir dir;
	{
		HistoryModel model(0, dir.name());
		model.addUrl(u1);
		QTest::qWait(2000);
		model.addUrl(u2);
		testModel(model, u2, u1);
	}

	kDebug() << "loading";
	HistoryModel model(0, dir.name());
	testModel(model, u2, u1);
}
