/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include <qtest_kde.h>

#include <QList>

#include "../lib/imagescaler.h"

#include "imagescalertest.moc"

QTEST_KDEMAIN( ImageScalerTest, GUI )

void ImageScalerTest::testScaledRect_data() {
	QTest::addColumn<QRectF>("input");
	QTest::addColumn<QRect>("expected");

	QTest::newRow("overflow right") << QRectF(1.0, 1.0, 2.7, 3.2) << QRect(1, 1, 3, 4);
	QTest::newRow("overflow left") << QRectF(0.5, 1.0, 2.0, 3.2) << QRect(0, 1, 3, 4);
	QTest::newRow("overflow both") << QRectF(0.5, 1.0, 2.6, 3.2) << QRect(0, 1, 4, 4);
}

void ImageScalerTest::testScaledRect() {
	QFETCH(QRectF, input);
	QFETCH(QRect, expected);
	QCOMPARE(Gwenview::ImageScaler::containingRect(input), expected);
}

/**
 * Scale whole image in one pass
 */
void ImageScalerTest::testScaleFullImage() {
	QImage image(10, 10, QImage::Format_ARGB32);
	const int zoom = 2;
	{
		QPainter painter(&image);
		painter.fillRect(image.rect(), Qt::white);
		painter.drawText(0, image.height(), "X");
	}

	Gwenview::ImageScaler scaler;
	scaler.setImage(image);
	scaler.setZoom(zoom);
	scaler.setRegion(QRect(QPoint(0,0), image.size() * zoom));

	QImage expectedImage = image.scaled( image.size() * zoom);

	ImageScalerClient client(&scaler);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	QImage scaledImage = client.createFullImage();
	QCOMPARE(scaledImage, expectedImage);
}


/**
 * Scale parts of an image
 * In this test, the result image should be missing its bottom-right corner
 */
void ImageScalerTest::testScalePartialImage() {
	QImage image(10, 10, QImage::Format_ARGB32);
	const int zoom = 2;
	{
		QPainter painter(&image);
		painter.fillRect(image.rect(), Qt::white);
		painter.drawText(0, image.height(), "X");
	}

	Gwenview::ImageScaler scaler;
	scaler.setImage(image);
	scaler.setZoom(zoom);
	scaler.setRegion(
		QRect(
			0, 0,
			image.width() * zoom / 2, image.height() * zoom)
		);
	scaler.addRegion(
		QRect(
			0, 0,
			image.width() * zoom, image.height() * zoom / 2)
		);

	QImage expectedImage(image.size() * zoom, image.format());
	expectedImage.fill(0);
	{
		QPainter painter(&expectedImage);
		QImage tmp;
		tmp= image.copy(0, 0,
			expectedImage.width() / zoom / 2,
			expectedImage.height() / zoom);
		painter.drawImage(0, 0, tmp.scaled(tmp.size() * zoom));
		tmp= image.copy(0, 0,
			expectedImage.width() / zoom,
			expectedImage.height() / zoom / 2);
		painter.drawImage(0, 0, tmp.scaled(tmp.size() * zoom));
	}

	ImageScalerClient client(&scaler);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	QImage scaledImage = client.createFullImage();

	QCOMPARE(scaledImage, expectedImage);
}



/**
 * Scale whole image in two passes, not using exact pixel boundaries
 */
void ImageScalerTest::testScaleFullImageTwoPasses() {
	QFETCH(qreal, zoom);
	QImage image(10, 10, QImage::Format_ARGB32);
	{
		QPainter painter(&image);
		painter.fillRect(image.rect(), Qt::white);
		painter.drawLine(0, 0, image.width(), image.height());
	}

	Gwenview::ImageScaler scaler;
	ImageScalerClient client(&scaler);

	scaler.setImage(image);
	scaler.setZoom(zoom);
	int zWidth = int(image.width() * zoom);
	int zHeight = int(image.width() * zoom);
	int partialZWidth = zWidth / 3;
	scaler.setRegion(
		QRect(
			0, 0,
			partialZWidth, zHeight)
		);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	scaler.addRegion(
		QRect(
			partialZWidth, 0,
			zWidth - partialZWidth, zHeight)
		);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	QImage expectedImage = image.scaled(image.size() * zoom);

	QImage scaledImage = client.createFullImage();
	QCOMPARE(expectedImage, scaledImage);
}


void ImageScalerTest::testScaleFullImageTwoPasses_data() {
	QTest::addColumn<qreal>("zoom");

	QTest::newRow("0.5") << 0.5;
	QTest::newRow("2.0") << 2.0;
	QTest::newRow("4.0") << 4.0;
}


/**
 * When zooming out, make sure that we don't crash when scaling an area which
 * have one dimension smaller than one pixel in the destination. 
 */
void ImageScalerTest::testScaleThinArea() {
	QImage image(10, 10, QImage::Format_ARGB32);
	image.fill(0);

	Gwenview::ImageScaler scaler;

	const qreal zoom = 0.25;
	scaler.setImage(image);
	scaler.setZoom(zoom);
	scaler.setRegion(QRect(0, 0, image.width(), 2));
	while (scaler.isRunning()) {
		QTest::qWait(30);
	}
}


void ImageScalerTest::testDontStartWithoutImage() {
	Gwenview::ImageScaler scaler;
	scaler.setZoom(1.0);
	scaler.setRegion(QRect(0, 0, 10, 10));
	QVERIFY(!scaler.isRunning());
}


/**
 * Test that scaling down a big image (==bigger than MAX_CHUNK_AREA) does not
 * produce any gap
 */
void ImageScalerTest::testScaleDownBigImage() {
	QImage image(1704, 2272, QImage::Format_RGB32);
	image.fill(255);

	Gwenview::ImageScaler scaler;
	ImageScalerClient client(&scaler);

	const qreal zoom = 0.28125;
	scaler.setImage(image);
	scaler.setZoom(zoom);
	scaler.setRegion(QRect( QPoint(0, 0), image.size() * zoom));
	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	QImage scaledImage = client.createFullImage();

	QImage expectedImage = image.scaled(scaledImage.size());
	QCOMPARE(expectedImage, scaledImage);
}
