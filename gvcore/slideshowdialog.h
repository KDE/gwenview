// vim: set tabstop=4 shiftwidth=4 noexpandtab
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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

#ifndef SLIDESHOWDIALOG_H
#define SLIDESHOWDIALOG_H

// KDE includes
#include <kdialogbase.h>
#include "libgwenview_export.h"
class SlideShowDialogBase;
namespace Gwenview {
class SlideShow;

class LIBGWENVIEW_EXPORT SlideShowDialog: public KDialogBase {
Q_OBJECT
public:
	SlideShowDialog(QWidget* parent,SlideShow*);

protected slots:
	void slotOk();
	
private:
	SlideShowDialogBase* mContent;
	SlideShow* mSlideShow;
};


} // namespace
#endif

