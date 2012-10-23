// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include <KDebug>
#include <KLocale>

// Local
#include "ramp.h"
#include "document/document.h"
#include "document/documentjob.h"
#include "document/abstractdocumenteditor.h"
#include "paintutils.h"

namespace Gwenview
{

class RedEyeReductionJob : public ThreadedDocumentJob
{
public:
    RedEyeReductionJob(const QRectF& rectF)
        : mRectF(rectF)
    {}

    void threadedStart()
    {
        if (!checkDocumentEditor()) {
            return;
        }
        QImage img = document()->image();
        RedEyeReductionImageOperation::apply(&img, mRectF);
        document()->editor()->setImage(img);
        setError(NoError);
    }

private:
    QRectF mRectF;
};

struct RedEyeReductionImageOperationPrivate
{
    QRectF mRectF;
    QImage mOriginalImage;
};

RedEyeReductionImageOperation::RedEyeReductionImageOperation(const QRectF& rectF)
: d(new RedEyeReductionImageOperationPrivate)
{
    d->mRectF = rectF;
    setText(i18n("RedEyeReduction"));
}

RedEyeReductionImageOperation::~RedEyeReductionImageOperation()
{
    delete d;
}

void RedEyeReductionImageOperation::redo()
{
    QImage img = document()->image();
    QRect rect = PaintUtils::containingRect(d->mRectF);
    d->mOriginalImage = img.copy(rect);
    redoAsDocumentJob(new RedEyeReductionJob(d->mRectF));
}

void RedEyeReductionImageOperation::undo()
{
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
 * This code is inspired from code found in a Paint.net plugin:
 * http://paintdotnet.forumer.com/viewtopic.php?f=27&t=26193&p=205954&hilit=red+eye#p205954
 */
inline qreal computeRedEyeAlpha(const QColor& src)
{
    int hue, sat, value;
    src.getHsv(&hue, &sat, &value);

    qreal axs = 1.0;
    if (hue > 259) {
        static const Ramp ramp(30, 35, 0., 1.);
        axs = ramp(sat);
    } else {
        const Ramp ramp(hue * 2 + 29, hue * 2 + 40, 0., 1.);
        axs = ramp(sat);
    }

    return qBound(qreal(0.), src.alphaF() * axs, qreal(1.));
}

void RedEyeReductionImageOperation::apply(QImage* img, const QRectF& rectF)
{
    const QRect rect = PaintUtils::containingRect(rectF);
    const qreal radius = rectF.width() / 2;
    const qreal centerX = rectF.x() + radius;
    const qreal centerY = rectF.y() + radius;
    const Ramp radiusRamp(
        qMin(qreal(radius * 0.7), qreal(radius - 1)), radius,
        qreal(1.), qreal(0.));

    uchar* line = img->scanLine(rect.top()) + rect.left() * 4;
    for (int y = rect.top(); y < rect.bottom(); ++y, line += img->bytesPerLine()) {
        QRgb* ptr = (QRgb*)line;

        for (int x = rect.left(); x < rect.right(); ++x, ++ptr) {
            const qreal currentRadius = sqrt(pow(y - centerY, 2) + pow(x - centerX, 2));
            qreal alpha = radiusRamp(currentRadius);
            if (qFuzzyCompare(alpha, 0)) {
                continue;
            }

            const QColor src(*ptr);
            alpha *= computeRedEyeAlpha(src);
            int r = src.red();
            int g = src.green();
            int b = src.blue();
            QColor dst;
            // Replace red with green, and blend according to alpha
            dst.setRed(int((1 - alpha) * r + alpha * g));
            dst.setGreen(g);
            dst.setBlue(b);
            *ptr = dst.rgba();
        }
    }
}

} // namespace
