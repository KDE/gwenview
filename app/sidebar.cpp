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
#include <QLabel>
#include <QVBoxLayout>

namespace Gwenview {


SideBarGroup::SideBarGroup(QWidget* parent, const QString& title)
: QFrame(parent) {
	new QVBoxLayout(this);
	setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

	QLabel* label = new QLabel(this);
	QFont font(label->font());
	font.setBold(true);
	label->setFont(font);
	label->setText(title);

	layout()->addWidget(label);
}


void SideBarGroup::addWidget(QWidget* widget) {
	widget->setParent(this);
	layout()->addWidget(widget);
}


SideBar::SideBar(QWidget* parent)
: QFrame(parent) {
	mLayout = new QVBoxLayout(this);
	setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	mLayout->addStretch();
}


void SideBar::clear() {
	Q_FOREACH(QWidget* widget, mGroupList) {
		delete widget;
	}
	mGroupList.clear();
}

SideBarGroup* SideBar::createGroup(const QString& title) {
	SideBarGroup* group = new SideBarGroup(this, title);
	mLayout->insertWidget(mLayout->count() - 1, group);

	mGroupList << group;
	return group;
}

} // namespace
