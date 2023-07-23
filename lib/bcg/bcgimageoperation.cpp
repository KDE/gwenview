/*
Gwenview: an image viewer
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
#include "bcgimageoperation.h"

// Qt
#include <QImage>

// KF
#include <KLocalizedString>

// Local
#include "document/abstractdocumenteditor.h"
#include "document/document.h"
#include "document/documentjob.h"
#include "gwenview_lib_debug.h"
#include "imageutils.h"

namespace Gwenview
{
class BCGJob : public ThreadedDocumentJob
{
public:
    BCGJob(const BCGImageOperation::BrightnessContrastGamma &bcg)
        : mBcg(bcg)
    {
    }

    void threadedStart() override
    {
        if (!checkDocumentEditor()) {
            return;
        }
        QImage img = document()->image();
        BCGImageOperation::apply(img, mBcg);
        document()->editor()->setImage(img);
        setError(NoError);
    }

private:
    BCGImageOperation::BrightnessContrastGamma mBcg;
};

struct BCGImageOperationPrivate {
    QImage mOriginalImage;
    BCGImageOperation::BrightnessContrastGamma mBcg;
};

BCGImageOperation::BCGImageOperation(const BrightnessContrastGamma &bcg)
    : d(new BCGImageOperationPrivate)
{
    setText(i18n("Adjust Colors"));

    d->mBcg = bcg;
}

BCGImageOperation::~BCGImageOperation()
{
    delete d;
}

void BCGImageOperation::redo()
{
    d->mOriginalImage = document()->image();
    redoAsDocumentJob(new BCGJob(d->mBcg));
}

void BCGImageOperation::undo()
{
    if (!document()->editor()) {
        qCWarning(GWENVIEW_LIB_LOG) << "!document->editor()";
        return;
    }
    document()->editor()->setImage(d->mOriginalImage);
    finish(true);
}

void BCGImageOperation::apply(QImage &img, const BrightnessContrastGamma &bcg)
{
    if (bcg.brightness != 0) {
        img = ImageUtils::changeBrightness(img, bcg.brightness);
    }
    if (bcg.contrast != 0) {
        img = ImageUtils::changeContrast(img, bcg.contrast + 100);
    }
    if (bcg.gamma != 0) {
        img = ImageUtils::changeGamma(img, bcg.gamma + 100);
    }
}

} // namespace

#include "moc_bcgimageoperation.cpp"
