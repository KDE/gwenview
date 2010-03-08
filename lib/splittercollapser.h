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
#ifndef SPLITTERCOLLAPSER_H
#define SPLITTERCOLLAPSER_H

// Qt
#include <QToolButton>

// KDE

// Local
#include <lib/gwenviewlib_export.h>

class QSplitter;

namespace Gwenview {


struct SplitterCollapserPrivate;
/**
 * A button which appears on the side of a splitter handle and allows easy
 * collapsing of the widget on the opposite side
 */
class GWENVIEWLIB_EXPORT SplitterCollapser : public QToolButton {
	Q_OBJECT
public:
	SplitterCollapser(QSplitter*, QWidget* widget);
	~SplitterCollapser();

	virtual QSize sizeHint() const;

protected:
	virtual bool eventFilter(QObject*, QEvent*);

	virtual bool event(QEvent *event);
	virtual void paintEvent(QPaintEvent*);
	virtual void showEvent(QShowEvent*);

private:
	SplitterCollapserPrivate* const d;

private Q_SLOTS:
	void slotClicked();
};


} // namespace

#endif /* SPLITTERCOLLAPSER_H */
