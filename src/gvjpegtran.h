/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau
 
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

#ifndef GVJPEGTRAN_H
#define GVJPEGTRAN_H

// Qt 
#include <qcstring.h>
#include <qobject.h>

// Local
#include "gvimageutils.h"

class QString;

class KConfig;
class KProcess;


class GVJPEGTran : public QObject {
Q_OBJECT
public:
	static QByteArray apply(const QByteArray&,GVImageUtils::Orientation);

	static void readConfig(KConfig*,const QString& group);
	static void writeConfig(KConfig*,const QString& group);

	static QString programPath();
	static void setProgramPath(const QString&);

private slots:
	void slotProcessExited();
	void slotReceivedStdout(KProcess*,char*,int);

private:
	GVJPEGTran();
	bool mDone;
	QByteArray mResult;
};


#endif
