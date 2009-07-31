// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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


namespace Gwenview {


class HudWidgetPrivate;
class GWENVIEWLIB_EXPORT HudWidget : public QFrame {
	Q_OBJECT
public:
	enum Option {
		OptionNone                 = 0,
		OptionCloseButton          = 1 << 1,
		// FIXME: Ugly
		OptionDoNotFollowChildSize = 1 << 2, /// Make it possible to resize the hudwidget independently of child size
		OptionOpaque               = 1 << 3
	};
	Q_DECLARE_FLAGS(Options, Option)

	HudWidget(QWidget* parent = 0);
	~HudWidget();

	void init(QWidget*, Options options);

	QWidget* mainWidget() const;

Q_SIGNALS:
	void closed();

private Q_SLOTS:
	void slotCloseButtonClicked();

private:
	HudWidgetPrivate* const d;
};


} // namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::HudWidget::Options)

#endif /* HUDWIDGET_H */
