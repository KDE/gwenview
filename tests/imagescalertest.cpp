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
	scaler.start();

	QImage expectedImage = image.scaled( image.size() * zoom);

	ImageScalerClient client(&scaler);

	while (scaler.isRunning()) {
		QTest::qWait(30);
	}

	QImage scaledImage = client.createFullImage();
	QCOMPARE(scaledImage, expectedImage);
}


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
	scaler.start();

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
	scaledImage.save("scaled.png", "PNG");
	expectedImage.save("expected.png", "PNG");
	QCOMPARE(scaledImage, expectedImage);
}
