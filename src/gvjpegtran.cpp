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

// Qt
#include <qcursor.h>
#include <qeventloop.h>

// KDE
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

// Local
#include "gvjpegtran.moc"


const char* CONFIG_PROGRAM_PATH="path";
const unsigned int CHUNK_SIZE=1024;

static QString sProgramPath;


GVJPEGTran::GVJPEGTran()
{}

void GVJPEGTran::writeChunk(KProcess* process) {
	//kdDebug() << "writeChunk\n";

	if (mSent>=mSrc.size()) {
		//kdDebug() << " all done\n";
		process->closeStdin();
		return;
	}
	unsigned int size=QMIN(mSrc.size()-mSent, CHUNK_SIZE);
	//kdDebug() << " sending " << size << " bytes\n";
	process->writeStdin( mSrc.data() + mSent, size );
	mSent+=size;
}

void GVJPEGTran::slotReceivedStdout(KProcess*,char* data,int length) {
	//kdDebug() << "slotReceivedStdout size:" << length << endl;
	uint oldSize=mDst.size();
	mDst.resize(oldSize+length);
	memcpy(mDst.data()+oldSize,data,length);
}


void GVJPEGTran::slotReceivedStderr(KProcess* process,char* data, int length) {
	//kdDebug() << "slotReceivedStderr size:" << length << endl;
	kdWarning() << "Error: " << QCString(data,length) << endl;
	mDst.resize(0);
	process->kill();
}


void GVJPEGTran::slotProcessExited() {
	//kdDebug() << "slotProcessExited" << endl;
#if QT_VERSION<0x030100
	kapp->exit_loop();
#else
	kapp->eventLoop()->exitLoop();
#endif
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
	
	connect(&process,SIGNAL(wroteStdin(KProcess*)),
		&obj,SLOT(writeChunk(KProcess*)) );
	
	connect(&process,SIGNAL(receivedStdout(KProcess*,char*,int)),
		&obj,SLOT(slotReceivedStdout(KProcess*,char*,int)) );
	
	connect(&process,SIGNAL(receivedStderr(KProcess*,char*,int)),
		&obj,SLOT(slotReceivedStderr(KProcess*,char*,int)) );
	
	connect(&process,SIGNAL(processExited(KProcess*)),
		&obj,SLOT(slotProcessExited()) );
	
	// Return an empty QByteArray on failure. GVPixmap will thus consider the
	// buffer as invalid and will fall back to lossy manipulations.
	if ( !process.start(KProcess::NotifyOnExit, KProcess::All) ) {
		KMessageBox::information(0L,
				i18n("Gwenview couldn't perform lossless image manipulation.\n"
					"Make sure that the jpegtran program is installed and that "
					"its path in the configuration dialog is correct"
					),QString::null,"jpegtran failed");
		return QByteArray();
	}
	
	obj.mSrc=src;
	obj.mSent=0;
	obj.writeChunk(&process);
	
	kapp->setOverrideCursor(QCursor(WaitCursor));
#if QT_VERSION<0x030100
	kapp->enter_loop();
#else
	kapp->eventLoop()->enterLoop();
#endif
	kapp->restoreOverrideCursor();
	
	return obj.mDst;
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

