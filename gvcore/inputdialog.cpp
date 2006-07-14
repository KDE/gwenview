// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurélien Gâteau

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
// Self
#include "inputdialog.moc"

// Qt
#include <qlabel.h>
#include <qvbox.h>

// KDE
#include <klineedit.h>

namespace Gwenview {

struct InputDialog::Private {
	KLineEdit* mLineEdit;
	QLabel* mLabel;
};

	
InputDialog::InputDialog(QWidget* parent)
: KDialogBase(parent, "InputDialog", true, QString::null,
	KDialogBase::Ok|KDialogBase::Cancel)
{
	d = new Private;
	QVBox* page = makeVBoxMainWidget();
	d->mLabel = new QLabel(page);

	d->mLineEdit = new KLineEdit(page);
	d->mLineEdit->setFocus();
	
	setMinimumWidth(350);

	connect(d->mLineEdit, SIGNAL(textChanged(const QString&)),
		this, SLOT(updateButtons()) );
}


InputDialog::~InputDialog() {
	delete d;
}


void InputDialog::setLabel(const QString& label) {
	d->mLabel->setText(label);
}


KLineEdit* InputDialog::lineEdit() const {
	return d->mLineEdit;
}


void InputDialog::updateButtons() {
	enableButtonOK( ! d->mLineEdit->text().isEmpty() );
}


} // namespace

