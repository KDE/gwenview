// vim: set tabstop=4 shiftwidth=4 noexpandtab:
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "vtabwidget.moc"

// Qt
#include <qwidgetstack.h>

// KDE
#include <kmultitabbar.h>

namespace Gwenview {


struct VTabWidget::Private {
	KMultiTabBar* mTabBar;
	QWidgetStack* mStack;
	bool mEmpty;
};

	
VTabWidget::VTabWidget(QWidget* parent)
: QWidget(parent)
{
	d=new Private;
	d->mEmpty=true;
	d->mTabBar=new KMultiTabBar(KMultiTabBar::Vertical, this);
	d->mTabBar->setPosition(KMultiTabBar::Left);
	d->mTabBar->setStyle(KMultiTabBar::KDEV3ICON);
	d->mStack=new QWidgetStack(this);
	QHBoxLayout* layout=new QHBoxLayout(this);
	layout->add(d->mTabBar);
	layout->add(d->mStack);
}


VTabWidget::~VTabWidget() {
	delete d;
}


void VTabWidget::addTab(QWidget* child, const QPixmap& pix, const QString& label) {
	int id=d->mStack->addWidget(child);
	d->mTabBar->appendTab(pix, id, label);
	connect(d->mTabBar->tab(id), SIGNAL(clicked(int)),
		this, SLOT(slotClicked(int)) );

	if (d->mEmpty) {
		d->mTabBar->tab(id)->setOn(true);
		d->mEmpty=false;
	}
}


void VTabWidget::slotClicked(int id) {
	d->mStack->raiseWidget(id);
	QPtrList<KMultiTabBarTab>* tabs=d->mTabBar->tabs();
	QPtrListIterator<KMultiTabBarTab> it(*tabs);
	for (; it.current(); ++it) {
		KMultiTabBarTab* tab=it.current();
		tab->setOn(tab->id()==id);
	}
}


} // namespace
