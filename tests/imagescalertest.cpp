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

#include "../lib/imagescaler.h"

#include "imagescalertest.moc"

QTEST_KDEMAIN( ImageScalerTest, GUI )

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
	expectedImage.save("expected.png", "PNG");
	image.save("image.png", "PNG");
	scaledImage.save("scaled.png", "PNG");

	QCOMPARE(scaledImage.size(), expectedImage.size());
	// The scaler may produce more pixels than necessary. This is not an error,
	// but it means we need to skip transparent pixels of expectedImage to
	// check.
	for(int y=0; y<expectedImage.height(); ++y) {
		for(int x=0; x<expectedImage.height(); ++x) {
			QRgb expectedPixel = expectedImage.pixel(x, y);
			if (qAlpha(expectedPixel) == 0) {
				continue;
			}
			QCOMPARE(scaledImage.pixel(x,y), expectedPixel);
		}
	}
}



/**
 * Scale whole image in two passes, not using exact pixel boundaries
 */
void ImageScalerTest::testScaleFullImageTwoPasses() {
	QImage image(10, 10, QImage::Format_ARGB32);
	const int zoom = 2;
	{
		QPainter painter(&image);
		painter.fillRect(image.rect(), Qt::white);
		painter.drawLine(0, 0, image.width(), image.height());
	}

	Gwenview::ImageScaler scaler;
	ImageScalerClient client(&scaler);

	scaler.setImage(image);
	scaler.setZoom(zoom);
	scaler.setRegion(
		QRect(
			0, 0,
			image.width() * zoom / 3, image.height() * zoom)
		);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	scaler.addRegion(
		QRect(
			image.width() * zoom / 3, 0,
			image.width() * zoom * 2 / 3, image.height() * zoom)
		);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	QImage expectedImage = image.scaled(image.size() * zoom);

	QImage scaledImage = client.createFullImage();

	QCOMPARE(expectedImage, scaledImage);
}
