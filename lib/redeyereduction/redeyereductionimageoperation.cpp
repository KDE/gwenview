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


void RedEyeReductionImageOperation::apply(QImage* img, const QRectF& rectF) {
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
}


} // namespace
