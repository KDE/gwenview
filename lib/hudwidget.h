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
#ifndef HUDWIDGET_H
#define HUDWIDGET_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QFrame>

// KDE

// Local

class QToolButton;

namespace Gwenview {


class HudWidgetPrivate;
class GWENVIEWLIB_EXPORT HudWidget : public QFrame {
	Q_OBJECT
public:
	enum Option {
		OptionNone,
		OptionCloseButton,
		OptionDragHandle
	};
	Q_DECLARE_FLAGS(Options, Option)

	HudWidget(QWidget* parent = 0);
	~HudWidget();

	void init(QWidget*, Options options);

	QWidget* mainWidget() const;

	QToolButton* closeButton() const;

protected:
	virtual void moveEvent(QMoveEvent*);
	virtual bool eventFilter(QObject*, QEvent* event);

private:
	HudWidgetPrivate* const d;
};


} // namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::HudWidget::Options)

#endif /* HUDWIDGET_H */
