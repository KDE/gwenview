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

#include <string.h> // For memcpy

// KDE includes
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

// Our includes
#include "gvjpegtran.moc"


const char* CONFIG_PROGRAM_PATH="path";

static QString sProgramPath;


GVJPEGTran::GVJPEGTran()
: mDone(false)
{
}

void GVJPEGTran::slotReceivedStdout(KProcess*,char* data,int length) {
	uint oldSize=mResult.size();
	mResult.resize(oldSize+length);
	memcpy(mResult.data()+oldSize,data,length);
}

void GVJPEGTran::slotProcessExited() {
	mDone=true;
}

QByteArray GVJPEGTran::apply(const QByteArray& src,GVImageUtils::Orientation orientation) {
	GVJPEGTran obj;
	KProcess process;
	process << sProgramPath << "-copy" << "all";
	
	switch (orientation) {
	case GVImageUtils::HFlip:
		process << "-flip" << "horizontal";
		break;
	case GVImageUtils::Rot180:
		process << "-rotate" << "180";
		break;
	case GVImageUtils::VFlip:
		process << "-flip" << "vertical";
		break;
	case GVImageUtils::Rot90HFlip:
		process << "-transpose";
		break;
	case GVImageUtils::Rot90:
		process << "-rotate" << "90";
		break;
	case GVImageUtils::Rot90VFlip:
		process << "-transverse";
		break;
	case GVImageUtils::Rot270:
		process << "-rotate" << "270";
		break;
	default:
		return src;
	}
	
	connect(&process,SIGNAL(processExited(KProcess*)),
		&obj,SLOT(slotProcessExited()) );
	
	connect(&process,SIGNAL(receivedStdout(KProcess*,char*,int)),
		&obj,SLOT(slotReceivedStdout(KProcess*,char*,int)) );

	// Return an empty QByteArray on failure. GVPixmap will thus consider the
	// buffer as invalid and will fall back to lossy manipulations.
	if ( !process.start(KProcess::NotifyOnExit,KProcess::All) ) {
		KMessageBox::information(0L,
				i18n("Gwenview couldn't perform lossless image manipulation.\n"
					"Make sure that the jpegtran program is installed and that "
					"its path in the configuration dialog is correct"
					),QString::null,"jpegtran failed");
		return QByteArray();
	}
	process.writeStdin( src.data(),src.size() );
	process.closeStdin();

	while (!obj.mDone) {
		kapp->processEvents();
	}

	return obj.mResult;
}


//---------------------------------------------------------------------------
//
// Config
//
//---------------------------------------------------------------------------
void GVJPEGTran::readConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	sProgramPath=config->readEntry(CONFIG_PROGRAM_PATH,"jpegtran");
}


void GVJPEGTran::writeConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	config->writeEntry(CONFIG_PROGRAM_PATH,sProgramPath);
}


void GVJPEGTran::setProgramPath(const QString& path) {
	sProgramPath=path;
}


QString GVJPEGTran::programPath() {
	return sProgramPath;
}

