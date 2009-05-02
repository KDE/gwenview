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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef SPLITTER_H
#define SPLITTER_H

// Qt
#include <QSplitter>
#include <QStyleOption>
#include <QStylePainter>


namespace Gwenview {


class SplitterHandle : public QSplitterHandle {
public:
	SplitterHandle(Qt::Orientation orientation, QSplitter* parent)
	: QSplitterHandle(orientation, parent) {}

protected:
	virtual void paintEvent(QPaintEvent*) {
		QStylePainter painter(this);

		QStyleOption opt;
		opt.initFrom(this);

		// Draw a thin styled line below splitter handle
		QStyleOption lineOpt = opt;
		const int lineSize = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
		const int margin = 4 * lineSize;

		if (orientation() == Qt::Horizontal) {
			lineOpt.rect = QRect(width() - lineSize, -margin, height(), height() + 2*margin);
			opt.state = QStyle::State_Horizontal;
			opt.rect.adjust(0, 0, -lineSize, 0);
		} else {
			lineOpt.rect = QRect(-margin, height() - lineSize, width() + 2*margin, height());
			opt.state = QStyle::State_None;
			opt.rect.adjust(0, 0, 0, -lineSize);
		}
		lineOpt.state |= QStyle::State_Sunken;
		painter.drawPrimitive(QStyle::PE_Frame, lineOpt);

		// Draw the normal splitter handle
		painter.drawControl(QStyle::CE_Splitter, opt);
	}
};


/**
 * Home made splitter to be able to define a custom handle:
 * We want to show a thin line between the splitter and the thumbnail bar but
 * we don't do it with css because "border-top:" forces a border around the
 * whole widget (Qt 4.4.0)
 */
class Splitter : public QSplitter {
public:
	Splitter(Qt::Orientation orientation, QWidget* parent)
	: QSplitter(orientation, parent) {
		const int lineSize = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
		setHandleWidth(handleWidth() + lineSize);
	}

protected:
	virtual QSplitterHandle* createHandle() {
		return new SplitterHandle(orientation(), this);
	}
};


} // namespace

#endif /* SPLITTER_H */
