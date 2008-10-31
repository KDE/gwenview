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
#ifndef FILTERCONTROLLER_H
#define FILTERCONTROLLER_H

// Qt
#include <QList>
#include <QObject>
#include <QWidget>

// KDE

// Local

class QAction;
class QFrame;

namespace Gwenview {

class SortedDirModel;


struct NameFilterWidgetPrivate;
class NameFilterWidget : public QWidget {
	Q_OBJECT
public:
	NameFilterWidget(SortedDirModel*);
	~NameFilterWidget();

private Q_SLOTS:
	void applyNameFilter();

private:
	NameFilterWidgetPrivate* const d;
};

class FilterControllerPrivate;
/**
 * This class manages the filter widgets in the filter frame and assign the
 * corresponding filters to the SortedDirModel
 */
class FilterController : public QObject {
	Q_OBJECT
public:
	FilterController(QFrame* filterFrame, SortedDirModel* model);
	~FilterController();

	QList<QAction*> actionList() const;

private Q_SLOTS:
	void addFilterByName();
	void addFilterByRating();
	void addFilterByTag();

private:
	FilterControllerPrivate* const d;
};


} // namespace

#endif /* FILTERCONTROLLER_H */
