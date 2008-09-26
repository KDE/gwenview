// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "redeyereductionimageoperation.h"

// Stdc
#include <math.h>

// Qt
#include <QImage>
#include <QPainter>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "document/document.h"
#include "document/abstractdocumenteditor.h"
#include "paintutils.h"

namespace Gwenview {


struct RedEyeReductionImageOperationPrivate {
	QRectF mRectF;
	QImage mOriginalImage;
};


RedEyeReductionImageOperation::RedEyeReductionImageOperation(const QRectF& rectF)
: d(new RedEyeReductionImageOperationPrivate) {
	d->mRectF = rectF;
	setText(i18n("RedEyeReduction"));
}


RedEyeReductionImageOperation::~RedEyeReductionImageOperation() {
	delete d;
}


void RedEyeReductionImageOperation::redo() {
	QImage img = document()->image();

	QRect rect = PaintUtils::containingRect(d->mRectF);
	d->mOriginalImage = img.copy(rect);
	apply(&img, d->mRectF);
	if (!document()->editor()) {
		kWarning() << "!document->editor()";
		return;
	}
	document()->editor()->setImage(img);
}


void RedEyeReductionImageOperation::undo() {
	if (!document()->editor()) {
		kWarning() << "!document->editor()";
		return;
	}
	QImage img = document()->image();
	{
		QPainter painter(&img);
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		QRect rect = PaintUtils::containingRect(d->mRectF);
		painter.drawImage(rect.topLeft(), d->mOriginalImage);
	}
	document()->editor()->setImage(img);
}


inline qreal computeRedEyeAlpha(const QColor& src) {
	// Fine-tune input parameters, we may drop this block in the future by writing the "good" values directly into the formulas
	int Amount1x = -10;
	int Amount2x = 2;

	double ai = (double)src.alphaF();

	int hue, sat, value;
	src.getHsv(&hue, &sat, &value);

	hue += Amount2x;
	if (hue > 360) {
		hue -= 360;
	}
	if (hue < 0) {
		hue += 360;
	}

	// STEPS OF PROCESS:
	// Take a copy of src surface,
	// "Cut out" the eye-red color using alpha channel,
	// Replace red channel values with green,
	// Blend the result with original

	// Create alpha multiplier from Hue (alpha multipliers will act as our scissors):
	// 1.0 for hue above 270; 0 for hue below 260; gradual 10 steps between hue 260 - 270
	double axh = 1.0;
	if(hue > 259 && hue < 270) {
		axh = (hue - 259.0) / 10.0;
	}


	// Create alpha multiplier from Saturation:
	// if hue > 259 then it is 1.0 for saturation above 45; 0 for saturation below 40; gradual 5 steps between 40 and 45
	// if hue < 260 then it is more complex "curve", based on combination of hue and saturation

	double axs = 1.0;
	if (hue > 259) {
		if (sat < Amount1x + 40) {
			axs = 0;
		}
		if (sat > Amount1x + 39 && sat < Amount1x + 45) {
			axs = (sat - ((double)Amount1x + 39.0)) / 5.0;
		}
	}

	if (hue < 260) {
		if (sat < hue * 2 + Amount1x + 40) {
			axs = 0;
		}
		if (sat > hue * 2 + Amount1x + 39 && sat < hue * 2 + Amount1x + 50) {
			axs = (sat - ((double)hue * 2.0 + (double)Amount1x + 39.0)) / 10.0;
		}
	}

	// Merge the alpha multipliers:
	// Calculate final alpha based on original and multipliers:
	return qBound(0., ai * axs * axh, 1.);
}


void RedEyeReductionImageOperation::apply(QImage* img, const QRectF& rectF) {
	#if 0
	const qreal radius = rectF.width() / 2;
	const qreal centerX = rectF.x() + radius;
	const qreal centerY = rectF.y() + radius;

	QRect rect = PaintUtils::containingRect(rectF);
	uchar* line = img->scanLine(rect.top()) + rect.left() * 4;
	const qreal shadeRadius = qMin(radius * 0.7, radius - 1);

	for (int y = rect.top(); y < rect.bottom(); ++y, line += img->bytesPerLine()) {
		QRgb* ptr = (QRgb*)line;
		for (int x = rect.left(); x < rect.right(); ++x, ++ptr) {
			const qreal currentRadius = sqrt(pow(y - centerY, 2) + pow(x - centerX, 2));
			qreal k;
			if (currentRadius > radius) {
				continue;
			}
			if (currentRadius > shadeRadius) {
				k = (radius - currentRadius) / (radius - shadeRadius);
			} else {
				k = 1;
			}

			QColor src(*ptr);
			QColor dst = src;
			dst.setRed(( src.green() + src.blue() ) / 2);
			int h, s, v, a;
			dst.getHsv(&h, &s, &v, &a);
			dst.setHsv(h, 0, v, a);

			dst.setRed(int((1 -k) * src.red() + k * dst.red()));
			dst.setGreen(int((1 -k) * src.green() + k * dst.green()));
			dst.setBlue(int((1 -k) * src.blue() + k * dst.blue()));

			*ptr = dst.rgba();
		}
	}
	#endif

	QRect rect = PaintUtils::containingRect(rectF);

	uchar* line = img->scanLine(rect.top()) + rect.left() * 4;
	for (int y = rect.top(); y < rect.bottom(); ++y, line += img->bytesPerLine()) {
		QRgb* ptr = (QRgb*)line;

		for (int x = rect.left(); x < rect.right(); ++x, ++ptr) {
			QColor src(*ptr);
			qreal alpha = computeRedEyeAlpha(src);
			int r = src.red();
			int g = src.green();
			int b = src.blue();
			QColor dst;
			dst.setRed  (int((1 - alpha) * r + alpha * g));
			dst.setGreen(int((1 - alpha) * g + alpha * g));
			dst.setBlue (int((1 - alpha) * b + alpha * b));
			*ptr = dst.rgba();
		}
	}
}


} // namespace
