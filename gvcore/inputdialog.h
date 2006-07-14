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
#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H   

// KDE
#include <kdialogbase.h>


class KLineEdit;


namespace Gwenview {


/**
 * An input dialog which give access to its line edit
 */
class InputDialog : public KDialogBase {
Q_OBJECT
public:
	InputDialog(QWidget* parent);
	~InputDialog();
	void setLabel(const QString& label);
	KLineEdit* lineEdit() const;

private slots:
	void updateButtons();

private:
	struct Private;
	Private* d;
};

} // namespace

#endif /* INPUTDIALOG_H */
