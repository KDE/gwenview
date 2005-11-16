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

class QString;
class QWidget;

class KURL;

namespace Gwenview {
/**
 * This namespace handles all steps of a file operation :
 * - asking the user what to do with a file
 * - performing the operation
 * - showing result dialogs
 */
namespace FileOperation {

void copyTo(const KURL::List&,QWidget* parent=0L);
void moveTo(const KURL::List&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
void linkTo(const KURL::List& srcURL,QWidget* parent);
void del(const KURL::List&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
void rename(const KURL&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
void openDropURLMenu(QWidget* parent, const KURL::List&, const KURL& target, bool* wasMoved=0L);

} // namespace

} // namespace
#endif

