// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "tooltipwidget.moc"

// Qt
#include <QPainter>
#include <QToolTip>

// KDE
#include <kdebug.h>
#include <kcolorscheme.h>

// Local

namespace Gwenview {


struct ToolTipWidgetPrivate {
	qreal mOpacity;
};


ToolTipWidget::ToolTipWidget(QWidget* parent)
: QLabel(parent)
, d(new ToolTipWidgetPrivate) {
	d->mOpacity = 1.;
	setAutoFillBackground(true);
	setPalette(QToolTip::palette());
	/*
	QPalette pal = QToolTip::palette();
	pal.setCurrentColorGroup(QPalette::Inactive);
	QColor fgColor = pal.color(QPalette::ToolTipText);
	QColor bg2Color = pal.brush(QPalette::ToolTipBase).color();
	QColor bg1Color = KColorScheme::shade(bg2Color, KColorScheme::LightShade, 0.2);

	setStyleSheet(QString(
		"background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		" stop: 0 %1, stop: 1 %2);"
		"border-radius: 5px;"
		"color: %3;"
		"padding: 0 2px;")
		.arg(bg1Color.name())
		.arg(bg2Color.name())
		.arg(fgColor.name())
		);
	kDebug() << styleSheet();
	*/
}


ToolTipWidget::~ToolTipWidget() {
	delete d;
}


qreal ToolTipWidget::opacity() const {
	return d->mOpacity;
}


void ToolTipWidget::setOpacity(qreal opacity) {
	d->mOpacity = opacity;
	update();
}


void ToolTipWidget::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setOpacity(d->mOpacity);
	painter.fillRect(rect(), palette().brush(QPalette::ToolTipBase));
	painter.setPen(palette().color(QPalette::ToolTipText));
	painter.drawText(rect(), text());
}


} // namespace
