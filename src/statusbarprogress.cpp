/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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

// Qt includes
#include <qlabel.h>
#include <qlayout.h>
#include <qstatusbar.h>

// KDE includes
#include <kprogress.h>

// Our includes
#include "statusbarprogress.moc"


StatusBarProgress::StatusBarProgress(QStatusBar* statusBar,QString text,int count)
: QWidget(statusBar), mStatusBar(statusBar)
{
	mLayout=new QHBoxLayout(this,0,0);
	mLabel=new QLabel(text,this);
	mProgress=new KProgress(count,this);
	mLabel->setMaximumHeight(fontMetrics().height());
	mProgress->setMinimumWidth(100);
	mProgress->setMaximumHeight(fontMetrics().height());
	mLayout->add(mLabel);
	mLayout->add(mProgress);
}


StatusBarProgress::~StatusBarProgress() {
}


void StatusBarProgress::show() {
	mStatusBar->addWidget(this);
	QWidget::show();
}


void StatusBarProgress::hide() {
	QWidget::hide();
	mStatusBar->removeWidget(this);
}

