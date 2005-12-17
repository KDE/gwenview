// vim: set tabstop=4 shiftwidth=4 noexpandtab
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
#ifndef FILEOPERATION_H
#define FILEOPERATION_H

// KDE
#include <kurl.h>
#include <kio/job.h>

#include "libgwenview_export.h"

class QPopupMenu;
class QWidget;

namespace Gwenview {
/**
 * This namespace handles all steps of a file operation :
 * - asking the user what to do with a file
 * - performing the operation
 * - showing result dialogs
 */
namespace FileOperation {

LIBGWENVIEW_EXPORT void copyTo(const KURL::List&,QWidget* parent=0L);
LIBGWENVIEW_EXPORT void moveTo(const KURL::List&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
LIBGWENVIEW_EXPORT void linkTo(const KURL::List& srcURL,QWidget* parent);
LIBGWENVIEW_EXPORT void del(const KURL::List&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
LIBGWENVIEW_EXPORT void rename(const KURL&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);


/**
 * @internal
 */
class DropMenuContext : public QObject {
Q_OBJECT
public:
	DropMenuContext(QObject* parent, const KURL::List& src, const KURL& dst, bool* wasMoved)
	: QObject(parent)
	, mSrc(src)
	, mDst(dst)
	, mWasMoved(wasMoved)
	{
		if (mWasMoved) *mWasMoved=false;
	}
	
public slots:
	void copy() {
		KIO::copy(mSrc, mDst, true);
	}

	void move() {
		KIO::move(mSrc, mDst, true);
		if (mWasMoved) *mWasMoved=true;
	}

	void link() {
		KIO::link(mSrc, mDst, true);
	}

private:
	KURL::List mSrc;
	KURL mDst;
	bool* mWasMoved;
};


LIBGWENVIEW_EXPORT void fillDropURLMenu(QPopupMenu*, const KURL::List&, const KURL& target, bool* wasMoved=0L);
LIBGWENVIEW_EXPORT void openDropURLMenu(QWidget* parent, const KURL::List&, const KURL& target, bool* wasMoved=0L);

} // namespace

} // namespace
#endif

