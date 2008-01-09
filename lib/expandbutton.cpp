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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "expandbutton.h"

// Qt
#include <QStyle>
#include <QStyleOption>
#include <QStylePainter>

namespace Gwenview {

static const int INDICATOR_SIZE = 9; // ### hardcoded in qcommonstyle.cpp

ExpandButton::ExpandButton(QWidget* parent)
: QToolButton(parent) {
	setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setAutoRaise(true);
	setCheckable(true);
}

QSize ExpandButton::sizeHint() const {
	QSize size = QToolButton::sizeHint();
	size.setWidth(size.width() + 2*INDICATOR_SIZE);
	size.setHeight(fontMetrics().height());
	return size;
}

void ExpandButton::paintEvent(QPaintEvent*) {
	QStylePainter painter(this);

	QStyleOption option;
	option.initFrom(this);

	// Paint branch indicator
	// Code comes from Designer, sheet_delegate.cpp
	QStyleOption branchOption;
	QRect r = option.rect;
	branchOption.rect = QRect(r.left() + INDICATOR_SIZE/2, r.top() + (r.height() - INDICATOR_SIZE)/2, INDICATOR_SIZE, INDICATOR_SIZE);
	branchOption.palette = option.palette;
	branchOption.state = QStyle::State_Children;

	if (isChecked()) {
		branchOption.state |= QStyle::State_Open;
	}

	style()->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &painter, this);

	// Paint text
	QRect textRect = rect();
	textRect.setLeft(INDICATOR_SIZE * 2);

	painter.drawItemText(textRect, Qt::TextShowMnemonic | Qt::AlignVCenter, option.palette, isEnabled(), text());
}


} // namespace
