// vim: set tabstop=4 shiftwidth=4 noexpandtab
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

// KDE
#include <kdialogbase.h>

// KIPI
namespace KIPI {
class PluginLoader;
}


namespace Gwenview {

class ConfigDialogPrivate;

class ConfigDialog : public KDialogBase {
Q_OBJECT
public:
	ConfigDialog(QWidget*, KIPI::PluginLoader*);
	~ConfigDialog();

signals:
	void settingsChanged();

protected slots:
	void slotOk();
	void slotApply();

private slots:
	void calculateCacheSize();
	void emptyCache();
	void onCacheEmptied(KIO::Job*);

private:
	ConfigDialogPrivate* d;
};



} // namespace
#endif

