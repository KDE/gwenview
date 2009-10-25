// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "importerconfigdialog.h"

// Qt

// KDE

// Local
#include "importerconfig.h"
#include "ui_importerconfigdialog.h"

namespace Gwenview {


struct ImporterConfigDialogPrivate : public Ui_ImporterConfigDialog {
};


ImporterConfigDialog::ImporterConfigDialog(QWidget* parent)
: KConfigDialog(parent, "Importer Settings", ImporterConfig::self())
, d(new ImporterConfigDialogPrivate) {
	QWidget* widget = new QWidget;
	d->setupUi(widget);
	setFaceType(KPageDialog::Plain);
	addPage(widget, QString());
}


ImporterConfigDialog::~ImporterConfigDialog() {
	delete d;
}


HelpMap


} // namespace
