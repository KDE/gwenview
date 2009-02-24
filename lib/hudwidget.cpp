// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "hudwidget.moc"

// Qt
#include <QHBoxLayout>
#include <QToolButton>

// KDE
#include <kiconloader.h>

// Local
#include "fullscreentheme.h"

namespace Gwenview {


struct HudWidgetPrivate {
	QWidget* mMainWidget;
	QToolButton* mCloseButton;
};


HudWidget::HudWidget(QWidget* parent)
: QFrame(parent)
, d(new HudWidgetPrivate) {
	d->mMainWidget = 0;
	d->mCloseButton = 0;
}


HudWidget::~HudWidget() {
	delete d;
}


void HudWidget::init(QWidget* mainWidget, Options options) {
	d->mMainWidget = mainWidget;
	d->mMainWidget->setParent(this);
	if (d->mMainWidget->layout()) {
		d->mMainWidget->layout()->setMargin(0);
	}

	FullScreenTheme theme(FullScreenTheme::currentThemeName());
	setStyleSheet(theme.styleSheet());

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(4);
	layout->addWidget(d->mMainWidget);

	if (options == OptionCloseButton) {
		d->mCloseButton = new QToolButton(this);
		d->mCloseButton->setAutoRaise(true);
		d->mCloseButton->setIcon(SmallIcon("window-close"));
		layout->addWidget(d->mCloseButton);
	}
}


QWidget* HudWidget::mainWidget() const {
	return d->mMainWidget;
}


QToolButton* HudWidget::closeButton() const {
	return d->mCloseButton;
}


} // namespace
