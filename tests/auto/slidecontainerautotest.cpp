// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#include "slidecontainerautotest.moc"

// Local
#include <lib/slidecontainer.h>

// KDE
#include <qtest_kde.h>

// Qt
#include <QTextEdit>
#include <QVBoxLayout>

QTEST_KDEMAIN(SlideContainerAutoTest, GUI)

using namespace Gwenview;

struct TestWindow : public QWidget {
    explicit TestWindow(QWidget* parent = 0)
	: QWidget(parent)
	, mContainer(new SlideContainer)
	, mContent(new QTextEdit)
	{
		mContent->setFixedSize(100, 40);
		mContainer->setContent(mContent);

		mMainWidget = new QTextEdit();
		QVBoxLayout* layout = new QVBoxLayout(this);
		layout->setSpacing(0);
		layout->setMargin(0);
		layout->addWidget(mMainWidget);
		layout->addWidget(mContainer);
	}

	SlideContainer* mContainer;
	QWidget* mMainWidget;
	QWidget* mContent;
};

void SlideContainerAutoTest::testInit() {
	TestWindow window;
	window.show();

	QTest::qWait(500);
	QVERIFY(!window.mContainer->isVisible());
	QCOMPARE(window.mMainWidget->height(), window.height());
}

void SlideContainerAutoTest::testSlideIn() {
	TestWindow window;
	window.show();

	window.mContainer->slideIn();
	while (window.mContainer->slideHeight() != window.mContent->height()) {
		QTest::qWait(100);
	}
	QVERIFY(window.mContainer->isVisible());
	QCOMPARE(window.mContainer->height(), window.mContent->height());
}

void SlideContainerAutoTest::testSlideOut() {
	TestWindow window;
	window.show();

	window.mContainer->slideIn();
	while (window.mContainer->slideHeight() != window.mContent->height()) {
		QTest::qWait(100);
	}
	window.mContainer->slideOut();
	while (window.mContainer->slideHeight() != 0) {
		QTest::qWait(100);
	}
	QVERIFY(!window.mContainer->isVisible());
	QCOMPARE(window.mContainer->height(), 0);
}

void SlideContainerAutoTest::testSlideIn2() {
	TestWindow window;
	window.show();

	window.mContainer->slideIn();
	while (window.mContainer->slideHeight() != window.mContent->height()) {
		QTest::qWait(100);
	}
	window.mContainer->slideOut();
	while (window.mContainer->slideHeight() != 0) {
		QTest::qWait(100);
	}
	QVERIFY(!window.mContainer->isVisible());
	QCOMPARE(window.mContainer->height(), 0);

	window.mContainer->slideIn();
	while (window.mContainer->slideHeight() != window.mContent->height()) {
		QTest::qWait(100);
	}
	QVERIFY(window.mContainer->isVisible());
	QCOMPARE(window.mContainer->height(), window.mContent->height());
}
