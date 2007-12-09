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
// Self
#include "slidecontainer.moc"


namespace Gwenview {


SlideContainer::SlideContainer(QWidget* parent)
: QFrame(parent) {
	mContent = 0;
	mTimeLine = new QTimeLine(500, this);
	connect(mTimeLine, SIGNAL(valueChanged(qreal)), SLOT(slotTimeLineChanged(qreal)) );
	connect(mTimeLine, SIGNAL(finished()), SLOT(slotTimeLineFinished()) );
	hide();
}


QWidget* SlideContainer::content() const {
	return mContent;
}


void SlideContainer::setContent(QWidget* content) {
	if (mContent) {
		mContent->setParent(0);
	}
	mContent = content;
	mContent->setParent(this);
	mContent->hide();
	mContent->adjustSize();
}


void SlideContainer::slideIn() {
	if (mTimeLine->direction() == QTimeLine::Backward) {
		mTimeLine->setDirection(QTimeLine::Forward);
	}
	if (!isVisible() && mTimeLine->state() == QTimeLine::NotRunning) {
		show();
		mTimeLine->start();
	}
}


void SlideContainer::slideOut() {
	if (mTimeLine->direction() == QTimeLine::Forward) {
		mTimeLine->setDirection(QTimeLine::Backward);
	}
	if (mTimeLine->state() == QTimeLine::NotRunning) {
		mTimeLine->start();
	}
}


void SlideContainer::slotTimeLineChanged(qreal value) {
	int posY = int( (value - 1.0) * height() );
	mContent->setGeometry(0, posY, width(), height());
	mContent->show();
}


void SlideContainer::slotTimeLineFinished() {
	if (mTimeLine->direction() == QTimeLine::Backward) {
		hide();
	}
}


QSize SlideContainer::sizeHint() const {
	if (mContent) {
		return mContent->sizeHint();
	} else {
		return QSize();
	}
}


QSize SlideContainer::minimumSizeHint() const {
	if (mContent) {
		return mContent->minimumSizeHint();
	} else {
		return QSize();
	}
}


void SlideContainer::resizeEvent(QResizeEvent*) {
	if (mContent) {
		mContent->resize(size());
	}
}

} // namespace
