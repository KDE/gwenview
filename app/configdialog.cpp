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
#include "configdialog.h"

// Qt

// KDE

// Local
#include "ui_configdialog.h"
#include <lib/gwenviewconfig.h>
#include <lib/invisiblebuttongroup.h>
#include <lib/scrolltool.h>

namespace Gwenview {


struct ConfigDialogPrivate : public Ui_ConfigDialog {
	InvisibleButtonGroup* mAlphaBackgroundModeGroup;
	InvisibleButtonGroup* mWheelBehaviorGroup;
};


ConfigDialog::ConfigDialog(QWidget* parent)
: KConfigDialog(parent, "Settings", GwenviewConfig::self())
, d(new ConfigDialogPrivate) {
	setFaceType(KPageDialog::Plain);
	QWidget* widget = new QWidget(this);
	d->setupUi(widget);
	widget->layout()->setMargin(0);

	d->mAlphaBackgroundModeGroup = new InvisibleButtonGroup(widget);
	d->mAlphaBackgroundModeGroup->setObjectName("kcfg_AlphaBackgroundMode");
	d->mAlphaBackgroundModeGroup->addButton(d->checkBoardRadioButton, int(ImageView::AlphaBackgroundCheckBoard));
	d->mAlphaBackgroundModeGroup->addButton(d->solidColorRadioButton, int(ImageView::AlphaBackgroundSolid));

	d->mWheelBehaviorGroup = new InvisibleButtonGroup(widget);
	d->mWheelBehaviorGroup->setObjectName("kcfg_MouseWheelBehavior");
	d->mWheelBehaviorGroup->addButton(d->mouseWheelScrollRadioButton, int(ScrollTool::MouseWheelScroll));
	d->mWheelBehaviorGroup->addButton(d->mouseWheelBrowseRadioButton, int(ScrollTool::MouseWheelBrowse));

	addPage(widget, "");
	setHelp(QString(), "gwenview");
}


ConfigDialog::~ConfigDialog() {
	delete d;
}


} // namespace
