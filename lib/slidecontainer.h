/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef SLIDECONTAINER_H
#define SLIDECONTAINER_H

// Qt
#include <QFrame>
#include <QTimeLine>

#include "gwenviewlib_export.h"

namespace Gwenview {


class GWENVIEWLIB_EXPORT SlideContainer : public QFrame {
	Q_OBJECT
public:
	SlideContainer(QWidget* parent = 0);

	QWidget* content() const;
	void setContent(QWidget* content);


public Q_SLOTS:
	void slideIn();
	void slideOut();

protected:
	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	void resizeEvent(QResizeEvent*);

private Q_SLOTS:
	void slotTimeLineChanged(qreal value);
	void slotTimeLineFinished();

private:
	QWidget* mContent;
	QTimeLine* mTimeLine;
};


} /* namespace */


#endif /* SLIDECONTAINER_H */
