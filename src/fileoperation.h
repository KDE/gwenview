/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau

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

class KConfig;
class KURL;


/**
 * This namespace-like class handles all steps of a file operation :
 * - asking the user what to do with a file
 * - performing the operation
 * - showing result dialogs
 */
class FileOperation {
public:
	static void copyTo(const KURL::List&,QWidget* parent=0L);
	static void moveTo(const KURL::List&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
	static void del(const KURL::List&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
	static void rename(const KURL&,QWidget* parent,QObject* receiver=0L,const char* slot=0L);
	static void openWithEditor(const KURL&);

// Config
	static void readConfig(KConfig*,const QString&);
	static void writeConfig(KConfig*,const QString&);

	static bool confirmDelete();
	static void setConfirmDelete(bool);

	static bool confirmCopy();
	static void setConfirmCopy(bool);

	static bool confirmMove();
	static void setConfirmMove(bool);

	static QString destDir();
	static void setDestDir(const QString&);

	static QString editor();
	static void setEditor(const QString&);
};

#endif
