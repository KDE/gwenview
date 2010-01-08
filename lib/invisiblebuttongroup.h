// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef INVISIBLEBUTTONGROUP_H
#define INVISIBLEBUTTONGROUP_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QWidget>

// KDE

// Local

class QAbstractButton;

namespace Gwenview {


struct InvisibleButtonGroupPrivate;
class GWENVIEWLIB_EXPORT InvisibleButtonGroup : public QWidget {
	Q_OBJECT
	Q_PROPERTY(int current READ selected WRITE setSelected)
public:
	explicit InvisibleButtonGroup(QWidget* parent = 0);
	~InvisibleButtonGroup();

	int selected() const;

	void addButton(QAbstractButton* button, int id);

public Q_SLOTS:
	void setSelected(int id);

Q_SIGNALS:
	void selectionChanged(int id);

private:
	InvisibleButtonGroupPrivate* const d;
};


} // namespace

#endif /* INVISIBLEBUTTONGROUP_H */
