// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef DOCUMENTVIEWCONTAINER_H
#define DOCUMENTVIEWCONTAINER_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt
#include <QPointer>
#include <QWidget>

class QPropertyAnimation;

namespace Gwenview {

class DocumentView;
class DocumentViewContainer;
class Placeholder;
class ViewItem;

class ViewItem : public QObject {
public:
	ViewItem(DocumentView* view, DocumentViewContainer* container);
	~ViewItem();
	Placeholder* createPlaceholder();
	DocumentView* view() const { return mView; }
	Placeholder* placeholder() const { return mPlaceholder; }

private:
	Q_DISABLE_COPY(ViewItem)

	DocumentView* mView;
	QPointer<Placeholder> mPlaceholder;
	DocumentViewContainer* mContainer;
};

/**
 * Draws a scaled screenshot of a widget. Faster than drawing the actual widget
 * when animating.
 */
class Placeholder : public QWidget {
	Q_OBJECT
	Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
	Placeholder(ViewItem* item, QWidget* parent);
	void animate(QPropertyAnimation*);

	Q_INVOKABLE qreal opacity() const;

public Q_SLOTS:
	void setOpacity(qreal);

Q_SIGNALS:
	void animationFinished(ViewItem*);

protected:
	void paintEvent(QPaintEvent*);

private:
	ViewItem* mViewItem;
	QPixmap mPixmap;
	qreal mOpacity;

private Q_SLOTS:
	void emitAnimationFinished();
};

class DocumentViewContainerPrivate;
/**
 * A container for DocumentViews which will arrange them to make best use of
 * available space.
 */
class GWENVIEWLIB_EXPORT DocumentViewContainer : public QWidget {
	Q_OBJECT
public:
	DocumentViewContainer(QWidget* parent=0);
	~DocumentViewContainer();

	void addView(DocumentView* view);

	/**
	 * Note: this method takes ownership of the view and will delete it
	 */
	void removeView(DocumentView* view);

protected:
	void showEvent(QShowEvent*);
	void resizeEvent(QResizeEvent*);

private Q_SLOTS:
	void updateLayout();
	void slotItemAnimationFinished(ViewItem*);

private:
	DocumentViewContainerPrivate* const d;
};


} // namespace

#endif /* DOCUMENTVIEWCONTAINER_H */
