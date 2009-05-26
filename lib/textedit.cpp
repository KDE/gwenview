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
#include "textedit.h"

// Qt
#include <QPainter>
#include <QTextDocument>

// KDE
#include <kdebug.h>
#include <kdialog.h>

// Local

namespace Gwenview {


struct TextEditPrivate {
	QString mClickMessage;
};


TextEdit::TextEdit(QWidget* parent)
: KTextEdit(parent)
, d(new TextEditPrivate) {
}


TextEdit::~TextEdit() {
	delete d;
}


void TextEdit::setClickMessage(const QString& message) {
	d->mClickMessage = message;
	viewport()->update();
}


QString TextEdit::clickMessage() const {
	return d->mClickMessage;
}


void TextEdit::focusInEvent(QFocusEvent* event) {
	viewport()->update();
	KTextEdit::focusInEvent(event);
}


void TextEdit::focusOutEvent(QFocusEvent* event) {
	viewport()->update();
	KTextEdit::focusOutEvent(event);
}


void TextEdit::paintEvent(QPaintEvent* event) {
	KTextEdit::paintEvent(event);
	if (hasFocus() || !document()->isEmpty() || d->mClickMessage.isEmpty()) {
		return;
	}

	QPainter painter(viewport());

	QColor color(viewport()->palette().color(foregroundRole()));
	color.setAlphaF(0.5);
	painter.setPen(color);

	int margin = KDialog::marginHint();
	QRect rect = frameRect().adjusted(margin, margin, -margin, -margin);
	painter.drawText(rect, Qt::AlignLeft|Qt::AlignTop, d->mClickMessage);
}


} // namespace
