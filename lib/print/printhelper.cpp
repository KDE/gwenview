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
#include "printhelper.h"
#include "dialogguard.h"

// STD
#include <memory>

// Qt
#include <QCheckBox>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QUrl>

// KF
#include <KLocalizedString>

// Local
#include "printoptionspage.h"

namespace Gwenview
{

struct PrintHelperPrivate
{
    QWidget* mParent;

    QSize adjustSize(PrintOptionsPage* optionsPage, Document::Ptr doc, int printerResolution, const QSize & viewportSize)
    {
        QSize size = doc->size();
        PrintOptionsPage::ScaleMode scaleMode = optionsPage->scaleMode();
        if (scaleMode == PrintOptionsPage::ScaleToPage) {
            bool imageBiggerThanPaper =
                size.width() > viewportSize.width()
                || size.height() > viewportSize.height();

            if (imageBiggerThanPaper || optionsPage->enlargeSmallerImages()) {
                size.scale(viewportSize, Qt::KeepAspectRatio);
            }

        } else if (scaleMode == PrintOptionsPage::ScaleToCustomSize) {
            double wImg = optionsPage->scaleWidth();
            double hImg = optionsPage->scaleHeight();
            size.setWidth(int(wImg * printerResolution));
            size.setHeight(int(hImg * printerResolution));

        } else {
            // No scale
            const double INCHES_PER_METER = 100. / 2.54;
            int dpmX = doc->image().dotsPerMeterX();
            int dpmY = doc->image().dotsPerMeterY();
            if (dpmX > 0 && dpmY > 0) {
                double wImg = double(size.width()) / double(dpmX) * INCHES_PER_METER;
                double hImg = double(size.height()) / double(dpmY) * INCHES_PER_METER;
                size.setWidth(int(wImg * printerResolution));
                size.setHeight(int(hImg * printerResolution));
            }
        }
        return size;
    }

    QPoint adjustPosition(PrintOptionsPage* optionsPage, const QSize& imageSize, const QSize & viewportSize)
    {
        Qt::Alignment alignment = optionsPage->alignment();
        int posX, posY;

        if (alignment & Qt::AlignLeft) {
            posX = 0;
        } else if (alignment & Qt::AlignHCenter) {
            posX = (viewportSize.width() - imageSize.width()) / 2;
        } else {
            posX = viewportSize.width() - imageSize.width();
        }

        if (alignment & Qt::AlignTop) {
            posY = 0;
        } else if (alignment & Qt::AlignVCenter) {
            posY = (viewportSize.height() - imageSize.height()) / 2;
        } else {
            posY = viewportSize.height() - imageSize.height();
        }

        return QPoint(posX, posY);
    }
};

PrintHelper::PrintHelper(QWidget* parent)
: d(new PrintHelperPrivate)
{
    d->mParent = parent;
}

PrintHelper::~PrintHelper()
{
    delete d;
}

void PrintHelper::print(Document::Ptr doc)
{
    doc->waitUntilLoaded();
    QPrinter printer;
    printer.setDocName(doc->url().fileName());

    auto* optionsPage = new PrintOptionsPage(doc->size());
    optionsPage->loadConfig();

    DialogGuard<QPrintDialog> dialog(&printer, d->mParent);
#if defined (Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    dialog->setOptionTabs(QList<QWidget*>() << optionsPage);
#else
    optionsPage->setParent(dialog.data());
#endif

    dialog->setWindowTitle(i18n("Print Image"));
    bool wantToPrint = dialog->exec();

    optionsPage->saveConfig();
    if (!wantToPrint) {
        return;
    }

    QPainter painter(&printer);
    QRect rect = painter.viewport();
    QSize size = d->adjustSize(optionsPage, doc, printer.resolution(), rect.size());
    QPoint pos = d->adjustPosition(optionsPage, size, rect.size());
    painter.setViewport(pos.x(), pos.y(), size.width(), size.height());

    QImage image = doc->image();
    painter.setWindow(image.rect());
    painter.drawImage(0, 0, image);
}

} // namespace
