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
#ifndef STATUSBARTOOLBUTTON_H
#define STATUSBARTOOLBUTTON_H

// Qt
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolButton>

namespace Gwenview {

/**
 * A tool button which can be grouped with another and look like one solid bar:
 *
 * ( button1 | button2 )
 */
class StatusBarToolButton : public QToolButton {
public:
	enum GroupPosition {
		GroupLeft,
		GroupRight,
		NotGrouped
	};

	StatusBarToolButton(QWidget* parent=0)
	: QToolButton(parent)
	, mGroupPosition(NotGrouped) {
		setToolButtonStyle(Qt::ToolButtonTextOnly);
		setFocusPolicy(Qt::NoFocus);
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	}

	virtual QSize minimumSizeHint() const {
		return sizeHint();
	}

	virtual QSize sizeHint() const {
		QSize sh = QToolButton::sizeHint();
		sh.setHeight(fontMetrics().height());
		return sh;
	}

	void setGroupPosition(StatusBarToolButton::GroupPosition groupPosition) {
		mGroupPosition = groupPosition;
	}

protected:
	virtual void paintEvent(QPaintEvent* event) {
		if (mGroupPosition == NotGrouped) {
			QToolButton::paintEvent(event);
			return;
		}
		QStylePainter painter(this);
		QStyleOptionToolButton opt;
		initStyleOption(&opt);
		QStyleOptionToolButton panelOpt = opt;

		// Panel
		QRect& panelRect = panelOpt.rect;
		switch (mGroupPosition) {
		case GroupLeft:
			panelRect.setWidth(panelRect.width() * 2);
			break;
		case GroupRight:
			panelRect.setLeft(panelRect.left() - panelRect.width());
			break;
		case NotGrouped:
			Q_ASSERT(0);
		}
		painter.drawPrimitive(QStyle::PE_PanelButtonTool, panelOpt);

		// Separator
		int x;
		QColor color;
		if (mGroupPosition == GroupRight) {
			color = opt.palette.color(QPalette::Light);
			x = opt.rect.left();
		} else {
			color = opt.palette.color(QPalette::Mid);
			x = opt.rect.right();
		}
		painter.setPen(color);
		int y1 = opt.rect.top() + 6;
		int y2 = opt.rect.bottom() - 6;
		painter.drawLine(x, y1, x, y2);

		// Text
		painter.drawControl(QStyle::CE_ToolButtonLabel, opt);
	}

private:
	GroupPosition mGroupPosition;
};

} // namespace

#endif /* STATUSBARTOOLBUTTON_H */
