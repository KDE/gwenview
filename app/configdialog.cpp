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
#include "configdialog.h"

// Qt

// KDE
#include <KLocalizedString>

// Local
#include <lib/gwenviewconfig.h>
#include <QFontDatabase>

namespace Gwenview
{

template <class Ui>
QWidget* setupPage(Ui& ui)
{
    QWidget* widget = new QWidget;
    ui.setupUi(widget);
    widget->layout()->setContentsMargins(0, 0, 0, 0);
    return widget;
}

ConfigDialog::ConfigDialog(QWidget* parent)
: KConfigDialog(parent, "Settings", GwenviewConfig::self())
{
    setFaceType(KPageDialog::List);

    QWidget* widget;
    KPageWidgetItem* pageItem;

    // General
    widget = setupPage(mGeneralConfigPage);
    
    mThumbnailActionsGroup = new InvisibleButtonGroup(widget);
    mThumbnailActionsGroup->setObjectName(QStringLiteral("kcfg_ThumbnailActions"));
    mThumbnailActionsGroup->addButton(mGeneralConfigPage.allButtonsThumbnailActionsRadioButton, int(ThumbnailActions::AllButtons));
    mThumbnailActionsGroup->addButton(mGeneralConfigPage.selectionOnlyThumbnailActionsRadioButton, int(ThumbnailActions::ShowSelectionButtonOnly));
    mThumbnailActionsGroup->addButton(mGeneralConfigPage.noneThumbnailActionsRadioButton, int(ThumbnailActions::None));

    pageItem = addPage(widget, i18n("General"));
    pageItem->setIcon(QIcon::fromTheme(QStringLiteral("gwenview")));
    connect(mGeneralConfigPage.kcfg_ViewBackgroundValue, &QAbstractSlider::valueChanged, this, &ConfigDialog::updateViewBackgroundFrame);
    connect(mGeneralConfigPage.kcfg_JPEGQuality, &QAbstractSlider::valueChanged, this, [=] (int value) {mGeneralConfigPage.jpegQualitySpinner->setValue(value);});
    connect(mGeneralConfigPage.jpegQualitySpinner, QOverload<int>::of(&QSpinBox::valueChanged), this, [=] (int value) {mGeneralConfigPage.kcfg_JPEGQuality->setValue(value);});
    mGeneralConfigPage.jpegQualitySpinner->setValue(mGeneralConfigPage.kcfg_JPEGQuality->value());
    mGeneralConfigPage.backgroundValueFrame->setMinimumWidth(mGeneralConfigPage.jpegQualitySpinner->width());
    
    // Image View
    widget = setupPage(mImageViewConfigPage);

    mAlphaBackgroundModeGroup = new InvisibleButtonGroup(widget);
    mAlphaBackgroundModeGroup->setObjectName(QStringLiteral("kcfg_AlphaBackgroundMode"));
    mAlphaBackgroundModeGroup->addButton(mImageViewConfigPage.surroundingRadioButton, int(AbstractImageView::AlphaBackgroundNone));
    mAlphaBackgroundModeGroup->addButton(mImageViewConfigPage.checkBoardRadioButton, int(AbstractImageView::AlphaBackgroundCheckBoard));
    mAlphaBackgroundModeGroup->addButton(mImageViewConfigPage.solidColorRadioButton, int(AbstractImageView::AlphaBackgroundSolid));

    mWheelBehaviorGroup = new InvisibleButtonGroup(widget);
    mWheelBehaviorGroup->setObjectName(QStringLiteral("kcfg_MouseWheelBehavior"));
    mWheelBehaviorGroup->addButton(mImageViewConfigPage.mouseWheelScrollRadioButton, int(MouseWheelBehavior::Scroll));
    mWheelBehaviorGroup->addButton(mImageViewConfigPage.mouseWheelBrowseRadioButton, int(MouseWheelBehavior::Browse));
    mWheelBehaviorGroup->addButton(mImageViewConfigPage.mouseWheelZoomRadioButton, int(MouseWheelBehavior::Zoom));

    mAnimationMethodGroup = new InvisibleButtonGroup(widget);
    mAnimationMethodGroup->setObjectName(QStringLiteral("kcfg_AnimationMethod"));
    mAnimationMethodGroup->addButton(mImageViewConfigPage.glAnimationRadioButton, int(DocumentView::GLAnimation));
    mAnimationMethodGroup->addButton(mImageViewConfigPage.softwareAnimationRadioButton, int(DocumentView::SoftwareAnimation));
    mAnimationMethodGroup->addButton(mImageViewConfigPage.noAnimationRadioButton, int(DocumentView::NoAnimation));

    mZoomModeGroup = new InvisibleButtonGroup(widget);
    mZoomModeGroup->setObjectName(QStringLiteral("kcfg_ZoomMode"));
    mZoomModeGroup->addButton(mImageViewConfigPage.autofitZoomModeRadioButton, int(ZoomMode::Autofit));
    mZoomModeGroup->addButton(mImageViewConfigPage.keepSameZoomModeRadioButton, int(ZoomMode::KeepSame));
    mZoomModeGroup->addButton(mImageViewConfigPage.individualZoomModeRadioButton, int(ZoomMode::Individual));

    mThumbnailBarOrientationGroup = new InvisibleButtonGroup(widget);
    mThumbnailBarOrientationGroup->setObjectName(QStringLiteral("kcfg_ThumbnailBarOrientation"));
    mThumbnailBarOrientationGroup->addButton(mImageViewConfigPage.horizontalRadioButton, int(Qt::Horizontal));
    mThumbnailBarOrientationGroup->addButton(mImageViewConfigPage.verticalRadioButton, int(Qt::Vertical));

    pageItem = addPage(widget, i18n("Image View"));
    pageItem->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-display-color")));

    // Advanced
    widget = setupPage(mAdvancedConfigPage);

    mRenderingIntentGroup = new InvisibleButtonGroup(widget);
    mRenderingIntentGroup->setObjectName(QStringLiteral("kcfg_RenderingIntent"));
    mRenderingIntentGroup->addButton(mAdvancedConfigPage.relativeRenderingIntentRadioButton, int(RenderingIntent::Relative));
    mRenderingIntentGroup->addButton(mAdvancedConfigPage.perceptualRenderingIntentRadioButton, int(RenderingIntent::Perceptual));

    pageItem = addPage(widget, i18n("Advanced"));
    pageItem->setIcon(QIcon::fromTheme(QStringLiteral("preferences-other")));
    mAdvancedConfigPage.cacheHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    mAdvancedConfigPage.perceptualHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    mAdvancedConfigPage.relativeHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    mAdvancedConfigPage.colorProfileHelpLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));


    updateViewBackgroundFrame();
}

void ConfigDialog::updateViewBackgroundFrame()
{
    const QColor color = QColor::fromHsv(0, 0, mGeneralConfigPage.kcfg_ViewBackgroundValue->value());
    const QString css =
        QStringLiteral(
            "background-color: %1;"
            "border-radius: 5px;"
            "border: 1px solid %1;")
        .arg(color.name());
    // When using Oxygen, setting the background color via palette causes the
    // pixels outside the frame to be painted with the new background color as
    // well. Using CSS works more like expected.
    mGeneralConfigPage.backgroundValueFrame->setStyleSheet(css);
}

} // namespace
