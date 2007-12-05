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
#include "sidebar.moc"

// Qt
#include <QAction>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <kglobalsettings.h>

// Local
#include "lib/expandbutton.h"

namespace Gwenview {


struct SideBarGroupPrivate {
	QFrame* mContainer;
	ExpandButton* mTitleButton;
};


SideBarGroup::SideBarGroup(QWidget* parent, const QString& title)
: QFrame(parent)
, d(new SideBarGroupPrivate) {
	d->mContainer = 0;
	d->mTitleButton = new ExpandButton(this);
	d->mTitleButton->setChecked(true);
	d->mTitleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->mTitleButton->setFixedHeight(d->mTitleButton->sizeHint().height() * 3 / 2);
	QFont font(d->mTitleButton->font());
	font.setBold(true);
	d->mTitleButton->setFont(font);
	d->mTitleButton->setText(title);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(d->mTitleButton);

	clear();
}


SideBarGroup::~SideBarGroup() {
	delete d;
}


void SideBarGroup::paintEvent(QPaintEvent* event) {
	QFrame::paintEvent(event);
	if (y() != 0) {
		// Draw a separator, but only if we are not the first group
		QPainter painter(this);
		QPen pen(palette().mid().color());
		painter.setPen(pen);
		painter.drawLine(rect().topLeft(), rect().topRight());
	}
}


void SideBarGroup::addWidget(QWidget* widget) {
	widget->setParent(d->mContainer);
	d->mContainer->layout()->addWidget(widget);
}


void SideBarGroup::clear() {
	delete d->mContainer;

	d->mContainer = new QFrame(this);
	d->mContainer->setBackgroundRole(QPalette::Base);
	QVBoxLayout* containerLayout = new QVBoxLayout(d->mContainer);
	containerLayout->setMargin(0);
	containerLayout->setSpacing(0);

	layout()->addWidget(d->mContainer);

	connect(d->mTitleButton, SIGNAL(toggled(bool)),
		d->mContainer, SLOT(setVisible(bool)) );
}


void SideBarGroup::addAction(QAction* action) {
	QToolButton* button = new QToolButton();
	button->setAutoRaise(true);
	button->setDefaultAction(action);
	button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	addWidget(button);
}


struct SideBar::Private {
	QVBoxLayout* mLayout;
	QList<SideBarGroup*> mGroupList;
	QWidget* mWidget;
};


SideBar::SideBar(QWidget* parent)
: QScrollArea(parent) {
	d.reset(new Private);

	d->mWidget = new QWidget(this);

	d->mLayout = new QVBoxLayout(d->mWidget);
	d->mLayout->setMargin(0);
	d->mLayout->setSpacing(0);
	d->mLayout->addStretch();

	setFrameStyle(QFrame::NoFrame);
	setWidget(d->mWidget);
	setWidgetResizable(true);
	setFont(KGlobalSettings::toolBarFont());
}


void SideBar::clear() {
	Q_FOREACH(QWidget* widget, d->mGroupList) {
		delete widget;
	}
	d->mGroupList.clear();
}

SideBarGroup* SideBar::createGroup(const QString& title) {
	SideBarGroup* group = new SideBarGroup(d->mWidget, title);
	d->mLayout->insertWidget(d->mLayout->count() - 1, group);

	d->mGroupList << group;
	return group;
}

QSize SideBar::sizeHint() const {
	return QSize(200, 200);
}


void SideBar::showEvent(QShowEvent* event) {
	aboutToShow();
	QScrollArea::showEvent(event);
}


} // namespace
