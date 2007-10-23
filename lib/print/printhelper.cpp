// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "printhelper.h"

// STD
#include <memory>

// Qt
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>

// KDE

// Local

namespace Gwenview {


struct PrintHelperPrivate {
	QWidget* mParent;
};


PrintHelper::PrintHelper(QWidget* parent)
: d(new PrintHelperPrivate) {
	d->mParent = parent;
}


PrintHelper::~PrintHelper() {
	delete d;
}


void PrintHelper::print(Document::Ptr doc) {
	/*PrintOptionsPage* optionsPage = new PrintOptionsPage(this);
	QPrinter printer;
	QPrintDialog* dialog = KdePrint::createPrintDialog(&printer,
		QList<QWidget*>() << optionsPage,
		this);*/
	QPrinter printer;
	std::auto_ptr<QPrintDialog> dialog( new QPrintDialog(&printer, d->mParent) );
	bool wantToPrint = dialog->exec();

	//optionsPage->saveSettings();
	if (!wantToPrint) {
		return;
	}

	doc->waitUntilLoaded();
	QImage image = doc->image();

	QPainter painter(&printer);
	QRect rect = painter.viewport();
	QSize size = image.size();
	size.scale(rect.size(), Qt::KeepAspectRatio);
	painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
	painter.setWindow(image.rect());
	painter.drawImage(0, 0, image);
}


} // namespace
