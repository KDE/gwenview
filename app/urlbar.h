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
#ifndef URLBAR_H
#define URLBAR_H   

// KDE
#include <klistview.h>

class QListViewItem;

class KConfig;
class KURL;

namespace Gwenview {

class URLBar : public KListView {
Q_OBJECT
public:
	URLBar(QWidget* parent);
	~URLBar();

	void readConfig(KConfig*, const QString& group);
	void writeConfig(KConfig*, const QString& group);

signals:
	void activated(const KURL&);

private slots:
	void slotClicked(QListViewItem*);
};

} // namespace

#endif /* URLBAR_H */
