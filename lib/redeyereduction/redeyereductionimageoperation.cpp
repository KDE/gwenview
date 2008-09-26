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


/**
 * This helper class maps values on a linear ramp.
 * It's useful to do mappings like this:
 *
 * x | -oo       x1               x2     +oo
 * --+--------------------------------------
 * y |  y1       y1 (linear ramp) y2      y2
 *
 * Note that y1 can be greater than y2 if necessary
 */
class Ramp {
public:
	Ramp(qreal x1, qreal x2, qreal y1, qreal y2)
	: mX1(x1)
	, mX2(x2)
	, mY1(y1)
	, mY2(y2)
	{
		mK = (y2 - y1) / (x2 - x1);
	}

	qreal operator()(qreal x) const {
		if (x < mX1) {
			return mY1;
		}
		if (x > mX2) {
			return mY2;
		}
		return mY1 + (x - mX1) * mK;
	}

private:
	qreal mX1, mX2, mY1, mY2, mK;
};


inline qreal computeRedEyeAlpha(const QColor& src) {
	// Fine-tune input parameters, we may drop this block in the future by writing the "good" values directly into the formulas
	const int Amount1x = -10;
	const int Amount2x = 2;

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

	// Create alpha multiplier from Saturation:
	// if hue > 259 then it is 1.0 for saturation above 45; 0 for saturation below 40; gradual 5 steps between 40 and 45
	// if hue < 260 then it is more complex "curve", based on combination of hue and saturation

	qreal axs = 1.0;
	if (hue > 259) {
		static const Ramp ramp(Amount1x + 40, Amount1x + 45, 0., 1.);
		axs = ramp(sat);
	} else {
		const int minHue = hue * 2 + Amount1x + 39;
		const int maxHue = hue * 2 + Amount1x + 50;
		if (sat <= minHue) {
			axs = 0;
		}
		if (sat > minHue && sat < maxHue) {
			axs = (sat - minHue) / 10.0;
		}
	}

	// Merge alphas
	return qBound(0., src.alphaF() * axs, 1.);
}


void RedEyeReductionImageOperation::apply(QImage* img, const QRectF& rectF) {
	const QRect rect = PaintUtils::containingRect(rectF);
	const qreal radius = rectF.width() / 2;
	const qreal centerX = rectF.x() + radius;
	const qreal centerY = rectF.y() + radius;
	const Ramp radiusRamp(qMin(radius * 0.7, radius - 1), radius, 1., 0.);

	uchar* line = img->scanLine(rect.top()) + rect.left() * 4;
	for (int y = rect.top(); y < rect.bottom(); ++y, line += img->bytesPerLine()) {
		QRgb* ptr = (QRgb*)line;

		for (int x = rect.left(); x < rect.right(); ++x, ++ptr) {
			QColor src(*ptr);

			const qreal currentRadius = sqrt(pow(y - centerY, 2) + pow(x - centerX, 2));
			qreal alpha = radiusRamp(currentRadius);
			if (qFuzzyCompare(alpha, 0)) {
				continue;
			}

			alpha *= computeRedEyeAlpha(src);
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
