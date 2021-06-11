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
#include "printoptionspage.h"

// Qt
#include <QButtonGroup>
#include <QGridLayout>
#include <QToolButton>

// KF
#include <KConfigDialogManager>

// Local
#include "gwenview_lib_debug.h"
#include <gwenviewconfig.h>
#include <signalblocker.h>
#include <ui_printoptionspage.h>

namespace Gwenview
{

static inline double unitToInches(PrintOptionsPage::Unit unit)
{
    if (unit == PrintOptionsPage::Inches) {
        return 1.;
    } else if (unit == PrintOptionsPage::Centimeters) {
        return 1 / 2.54;
    } else { // Millimeters
        return 1 / 25.4;
    }
}

struct PrintOptionsPagePrivate : public Ui_PrintOptionsPage
{
    QSize mImageSize;
    QButtonGroup mScaleGroup;
    QButtonGroup mPositionGroup;
    KConfigDialogManager* mConfigDialogManager;

    void initPositionFrame()
    {
        mPositionFrame->setStyleSheet(
            QStringLiteral("QFrame {"
            "	background-color: palette(mid);"
            "	border: 1px solid palette(dark);"
            "}"
            "QToolButton {"
            "	border: none;"
            "	background: palette(base);"
            "}"
            "QToolButton:hover {"
            "	background: palette(alternate-base);"
            "	border: 1px solid palette(highlight);"
            "}"
            "QToolButton:checked {"
            "	background-color: palette(highlight);"
            "}")
        );

        auto* layout = new QGridLayout(mPositionFrame);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(1);
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                auto* button = new QToolButton(mPositionFrame);
                button->setFixedSize(40, 40);
                button->setCheckable(true);
                layout->addWidget(button, row, col);

                Qt::Alignment alignment;
                if (row == 0) {
                    alignment = Qt::AlignTop;
                } else if (row == 1) {
                    alignment = Qt::AlignVCenter;
                } else {
                    alignment = Qt::AlignBottom;
                }
                if (col == 0) {
                    alignment |= Qt::AlignLeft;
                } else if (col == 1) {
                    alignment |= Qt::AlignHCenter;
                } else {
                    alignment |= Qt::AlignRight;
                }

                mPositionGroup.addButton(button, int(alignment));
            }
        }
    }
};

PrintOptionsPage::PrintOptionsPage(const QSize& imageSize)
: QWidget()
, d(new PrintOptionsPagePrivate)
{
    d->setupUi(this);
    d->mImageSize = imageSize;
    d->mConfigDialogManager = new KConfigDialogManager(this, GwenviewConfig::self());

    d->initPositionFrame();

    d->mScaleGroup.addButton(d->mNoScale, NoScale);
    d->mScaleGroup.addButton(d->mScaleToPage, ScaleToPage);
    d->mScaleGroup.addButton(d->mScaleTo, ScaleToCustomSize);

    connect(d->kcfg_PrintWidth, SIGNAL(valueChanged(double)),
            SLOT(adjustHeightToRatio()));

    connect(d->kcfg_PrintHeight, SIGNAL(valueChanged(double)),
            SLOT(adjustWidthToRatio()));

    connect(d->kcfg_PrintKeepRatio, &QAbstractButton::toggled,
            this, &PrintOptionsPage::adjustHeightToRatio);
}

PrintOptionsPage::~PrintOptionsPage()
{
    delete d;
}

Qt::Alignment PrintOptionsPage::alignment() const
{
    int id = d->mPositionGroup.checkedId();
    qCWarning(GWENVIEW_LIB_LOG) << "alignment=" << id;
    return Qt::Alignment(id);
}

PrintOptionsPage::ScaleMode PrintOptionsPage::scaleMode() const
{
    return PrintOptionsPage::ScaleMode(d->mScaleGroup.checkedId());
}

bool PrintOptionsPage::enlargeSmallerImages() const
{
    return d->kcfg_PrintEnlargeSmallerImages->isChecked();
}

PrintOptionsPage::Unit PrintOptionsPage::scaleUnit() const
{
    return PrintOptionsPage::Unit(d->kcfg_PrintUnit->currentIndex());
}

double PrintOptionsPage::scaleWidth() const
{
    return d->kcfg_PrintWidth->value() * unitToInches(scaleUnit());
}

double PrintOptionsPage::scaleHeight() const
{
    return d->kcfg_PrintHeight->value() * unitToInches(scaleUnit());
}

void PrintOptionsPage::adjustWidthToRatio()
{
    if (!d->kcfg_PrintKeepRatio->isChecked()) {
        return;
    }
    double width = d->mImageSize.width() * d->kcfg_PrintHeight->value() / d->mImageSize.height();

    SignalBlocker blocker(d->kcfg_PrintWidth);
    d->kcfg_PrintWidth->setValue(width ? width : 1.);
}

void PrintOptionsPage::adjustHeightToRatio()
{
    if (!d->kcfg_PrintKeepRatio->isChecked()) {
        return;
    }
    double height = d->mImageSize.height() * d->kcfg_PrintWidth->value() / d->mImageSize.width();

    SignalBlocker blocker(d->kcfg_PrintHeight);
    d->kcfg_PrintHeight->setValue(height ? height : 1.);
}

void PrintOptionsPage::loadConfig()
{
    QAbstractButton* button;

    button = d->mPositionGroup.button(GwenviewConfig::printPosition());
    if (button) {
        button->setChecked(true);
    } else {
        qCWarning(GWENVIEW_LIB_LOG) << "Unknown button for position group";
    }

    button = d->mScaleGroup.button(GwenviewConfig::printScaleMode());
    if (button) {
        button->setChecked(true);
    } else {
        qCWarning(GWENVIEW_LIB_LOG) << "Unknown button for scale group";
    }

    d->mConfigDialogManager->updateWidgets();

    if (d->kcfg_PrintKeepRatio->isChecked()) {
        adjustHeightToRatio();
    }
}

void PrintOptionsPage::saveConfig()
{
    int position = d->mPositionGroup.checkedId();
    GwenviewConfig::setPrintPosition(position);

    auto scaleMode = ScaleMode(d->mScaleGroup.checkedId());
    GwenviewConfig::setPrintScaleMode(scaleMode);

    d->mConfigDialogManager->updateSettings();

    GwenviewConfig::self()->save();
}

} // namespace
