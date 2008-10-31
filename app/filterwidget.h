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
#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <config-gwenview.h>

// Qt
#include <QWidget>

// KDE

// Local

namespace Gwenview {


class SortedDirModel;

class AbstractFilterController : public QObject {
	Q_OBJECT
public:
	AbstractFilterController(QObject*);

	virtual void setDirModel(SortedDirModel* model);

	virtual void init(QWidget*) {};
	virtual void reset() = 0;
	virtual QWidget* widget() const = 0;

protected:
	SortedDirModel* mDirModel;
};


#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
class TagControllerPrivate;
class TagController : public AbstractFilterController {
	Q_OBJECT
public:
	TagController(QObject*);
	~TagController();

	virtual void init(QWidget*);
	virtual void reset();
	virtual QWidget* widget() const;

private Q_SLOTS:
	void updateTagSetFilter();

private:
	TagControllerPrivate* const d;
};
#endif


class FilterWidgetPrivate;
class FilterWidget : public QWidget {
	Q_OBJECT
public:
	FilterWidget(QWidget*);
	~FilterWidget();

	void setDirModel(SortedDirModel*);

public Q_SLOTS:
	void activateSelectedController(int index);

private:
	FilterWidgetPrivate* const d;
};


} // namespace

#endif /* FILTERWIDGET_H */
