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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

// Qt

// KF
#include <KConfigDialog>

// Local
#include "ui_generalconfigpage.h"
#include "ui_imageviewconfigpage.h"
#include "ui_advancedconfigpage.h"
#include <lib/invisiblebuttongroup.h>

namespace Gwenview
{

class ConfigDialog : public KConfigDialog
{
    Q_OBJECT
public:
    ConfigDialog(QWidget* parent);

private Q_SLOTS:
    void updateViewBackgroundFrame();

private:
    InvisibleButtonGroup* mWrapNavigationBehaviorGroup = nullptr;
    InvisibleButtonGroup* mAlphaBackgroundModeGroup = nullptr;
    InvisibleButtonGroup* mWheelBehaviorGroup = nullptr;
    InvisibleButtonGroup* mAnimationMethodGroup = nullptr;
    InvisibleButtonGroup* mFullScreenBackgroundGroup = nullptr;
    InvisibleButtonGroup* mThumbnailActionsGroup = nullptr;
    InvisibleButtonGroup* mZoomModeGroup = nullptr;
    InvisibleButtonGroup* mThumbnailBarOrientationGroup = nullptr;
    InvisibleButtonGroup* mRenderingIntentGroup = nullptr;
    Ui_GeneralConfigPage mGeneralConfigPage;
    Ui_ImageViewConfigPage mImageViewConfigPage;
    Ui_AdvancedConfigPage mAdvancedConfigPage;
};

} // namespace

#endif /* CONFIGDIALOG_H */
