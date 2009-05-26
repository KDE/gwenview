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
#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE
#include <ktextedit.h>

// Local

namespace Gwenview {


class TextEditPrivate;
/**
 * This class adds support for click message to KTextEdit
 * TODO: Merge with KTextEdit
 */
class GWENVIEWLIB_EXPORT TextEdit : public KTextEdit {
public:
	TextEdit(QWidget* parent = 0);
	~TextEdit();

	void setClickMessage(const QString& message);
	QString clickMessage() const;

protected:
	virtual void focusInEvent(QFocusEvent*);
	virtual void focusOutEvent(QFocusEvent*);
	virtual void paintEvent(QPaintEvent*);

private:
	TextEditPrivate* const d;
};


} // namespace

#endif /* TEXTEDIT_H */
