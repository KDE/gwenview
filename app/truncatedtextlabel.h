// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef TRUNCATEDTEXTLABEL_H
#define TRUNCATEDTEXTLABEL_H   

// Qt
#include <qlabel.h>
#include <qtooltip.h>

// KDE
#include <kwordwrap.h>

namespace Gwenview {

/**
 * A label which truncates it's text if it's too long, drawing it using
 * KWordWrap::drawFadeoutText()
 */
class TruncatedTextLabel : public QLabel {
public:
	TruncatedTextLabel(QWidget* parent)
	: QLabel(parent) {}

	QSize minimumSizeHint() const {
		QSize size=QLabel::minimumSizeHint();
		size.setWidth(-1);
		return size;
	}

	QSize sizeHint() const {
		return QSize(contentsRect().width(), QLabel::sizeHint().height());
	}

	void setText(const QString& text) {
		QLabel::setText(text);
		updateToolTip();
	}

protected:
	void drawContents(QPainter* painter) {
		KWordWrap::drawFadeoutText(painter, 0, fontMetrics().ascent(), width(), text());
	}

	void resizeEvent(QResizeEvent*) {
		updateToolTip();
	}

	void updateToolTip() {
		QString txt=text();
		QToolTip::remove(this);
		if ( width() < fontMetrics().width(txt) ) {
			QToolTip::add(this, txt);
		} else {
			QToolTip::hide();
		}
	}
};

} // namespace

#endif /* TRUNCATEDTEXTLABEL_H */
