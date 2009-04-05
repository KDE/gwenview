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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef ABSTRACTCONTEXTMANAGERITEM_H
#define ABSTRACTCONTEXTMANAGERITEM_H

// Qt
#include <QEvent>
#include <QObject>
#include <QWidget>


namespace Gwenview {

class AboutToShowHelper : public QObject {
	Q_OBJECT
public:
	AboutToShowHelper(QWidget* watched, QObject* receiver, const char* slot)
	: QObject(receiver) {
		Q_ASSERT(watched);
		watched->installEventFilter(this);
		connect(this, SIGNAL(aboutToShow()), receiver, slot);
	}

Q_SIGNALS:
	void aboutToShow();

protected:
	virtual bool eventFilter(QObject*, QEvent* event) {
		if (event->type() == QEvent::Show) {
			aboutToShow();
		}
		return false;
	}
};


class ContextManager;

struct AbstractContextManagerItemPrivate;
class AbstractContextManagerItem : public QObject {
	Q_OBJECT
public:
	AbstractContextManagerItem(ContextManager*);
	virtual ~AbstractContextManagerItem();

	QWidget* widget() const;

	ContextManager* contextManager() const;

protected:
	void setWidget(QWidget* widget);

private:
	AbstractContextManagerItemPrivate * const d;
};

} // namespace

#endif /* ABSTRACTCONTEXTMANAGERITEM_H */
