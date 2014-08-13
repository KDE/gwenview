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
#include <KGlobalSettings>
#include <KLocalizedString>

// Local
#include "ui_generalconfigpage.h"
#include "ui_imageviewconfigpage.h"
#include "ui_advancedconfigpage.h"
#include <lib/gwenviewconfig.h>
#include <lib/invisiblebuttongroup.h>

namespace Gwenview
{

struct ConfigDialogPrivate
{
    InvisibleButtonGroup* mAlphaBackgroundModeGroup;
    InvisibleButtonGroup* mWheelBehaviorGroup;
    InvisibleButtonGroup* mAnimationMethodGroup;
    InvisibleButtonGroup* mZoomModeGroup;
    InvisibleButtonGroup* mThumbnailBarOrientationGroup;
    Ui_GeneralConfigPage mGeneralConfigPage;
    Ui_ImageViewConfigPage mImageViewConfigPage;
    Ui_AdvancedConfigPage mAdvancedConfigPage;
};

template <class Ui>
QWidget* setupPage(Ui& ui)
{
    QWidget* widget = new QWidget;
    ui.setupUi(widget);
    widget->layout()->setMargin(0);
    return widget;
}

ConfigDialog::ConfigDialog(QWidget* parent)
: KConfigDialog(parent, "Settings", GwenviewConfig::self())
, d(new ConfigDialogPrivate)
{
    setFaceType(KPageDialog::List);

    QWidget* widget;
    KPageWidgetItem* pageItem;

    // General
    widget = setupPage(d->mGeneralConfigPage);
    pageItem = addPage(widget, i18n("General"));
    pageItem->setIcon(QIcon::fromTheme("gwenview"));
    connect(d->mGeneralConfigPage.kcfg_ViewBackgroundValue, SIGNAL(valueChanged(int)), SLOT(updateViewBackgroundFrame()));

    // Image View
    widget = setupPage(d->mImageViewConfigPage);

    d->mAlphaBackgroundModeGroup = new InvisibleButtonGroup(widget);
    d->mAlphaBackgroundModeGroup->setObjectName(QLatin1String("kcfg_AlphaBackgroundMode"));
    d->mAlphaBackgroundModeGroup->addButton(d->mImageViewConfigPage.checkBoardRadioButton, int(RasterImageView::AlphaBackgroundCheckBoard));
    d->mAlphaBackgroundModeGroup->addButton(d->mImageViewConfigPage.solidColorRadioButton, int(RasterImageView::AlphaBackgroundSolid));

    d->mWheelBehaviorGroup = new InvisibleButtonGroup(widget);
    d->mWheelBehaviorGroup->setObjectName(QLatin1String("kcfg_MouseWheelBehavior"));
    d->mWheelBehaviorGroup->addButton(d->mImageViewConfigPage.mouseWheelScrollRadioButton, int(MouseWheelBehavior::Scroll));
    d->mWheelBehaviorGroup->addButton(d->mImageViewConfigPage.mouseWheelBrowseRadioButton, int(MouseWheelBehavior::Browse));

    d->mAnimationMethodGroup = new InvisibleButtonGroup(widget);
    d->mAnimationMethodGroup->setObjectName(QLatin1String("kcfg_AnimationMethod"));
    d->mAnimationMethodGroup->addButton(d->mImageViewConfigPage.glAnimationRadioButton, int(DocumentView::GLAnimation));
    d->mAnimationMethodGroup->addButton(d->mImageViewConfigPage.softwareAnimationRadioButton, int(DocumentView::SoftwareAnimation));
    d->mAnimationMethodGroup->addButton(d->mImageViewConfigPage.noAnimationRadioButton, int(DocumentView::NoAnimation));

    d->mZoomModeGroup = new InvisibleButtonGroup(widget);
    d->mZoomModeGroup->setObjectName(QLatin1String("kcfg_ZoomMode"));
    d->mZoomModeGroup->addButton(d->mImageViewConfigPage.autofitZoomModeRadioButton, int(ZoomMode::Autofit));
    d->mZoomModeGroup->addButton(d->mImageViewConfigPage.keepSameZoomModeRadioButton, int(ZoomMode::KeepSame));
    d->mZoomModeGroup->addButton(d->mImageViewConfigPage.individualZoomModeRadioButton, int(ZoomMode::Individual));

    d->mThumbnailBarOrientationGroup = new InvisibleButtonGroup(widget);
    d->mThumbnailBarOrientationGroup->setObjectName(QLatin1String("kcfg_ThumbnailBarOrientation"));
    d->mThumbnailBarOrientationGroup->addButton(d->mImageViewConfigPage.horizontalRadioButton, int(Qt::Horizontal));
    d->mThumbnailBarOrientationGroup->addButton(d->mImageViewConfigPage.verticalRadioButton, int(Qt::Vertical));

    pageItem = addPage(widget, i18n("Image View"));
    pageItem->setIcon(QIcon::fromTheme("view-preview"));

    // Advanced
    widget = setupPage(d->mAdvancedConfigPage);
    pageItem = addPage(widget, i18n("Advanced"));
    pageItem->setIcon(QIcon::fromTheme("preferences-other"));
    d->mAdvancedConfigPage.cacheHelpLabel->setFont(KGlobalSettings::smallestReadableFont());

    updateViewBackgroundFrame();
}

ConfigDialog::~ConfigDialog()
{
    delete d;
}

void ConfigDialog::updateViewBackgroundFrame()
{
    QColor color = QColor::fromHsv(0, 0, d->mGeneralConfigPage.kcfg_ViewBackgroundValue->value());
    QString css =
        QString(
            "background-color: %1;"
            "border-radius: 5px;"
            "border: 1px solid %1;")
        .arg(color.name());
    // When using Oxygen, setting the background color via palette causes the
    // pixels outside the frame to be painted with the new background color as
    // well. Using CSS works more like expected.
    d->mGeneralConfigPage.backgroundValueFrame->setStyleSheet(css);
}

} // namespace
