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
#ifndef GVDOCUMENTDECODEIMPL_H
#define GVDOCUMENTDECODEIMPL_H

// Qt
#include <qasyncimageio.h>

// Local 
#include "tsthread/tsthread.h"
#include "gvdocumentimpl.h"

class GVDocument;

class GVDocumentDecodeImplPrivate;


class GVDecoderThread : public TSThread {
Q_OBJECT
public:
	void setRawData(const QByteArray&);
	QImage popLoadedImage();
	
signals:
	void failed();
	void succeeded();
	
protected:
	void run();

private:
	QMutex mMutex;
	QByteArray mRawData;
	QImage mImage;
};


class GVDocumentDecodeImpl : public GVDocumentImpl, public QImageConsumer {
Q_OBJECT
public:
	GVDocumentDecodeImpl(GVDocument* document);
	~GVDocumentDecodeImpl();
	
	void suspendLoading();
	void resumeLoading();

private:
	GVDocumentDecodeImplPrivate* d;
	void finish(QImage&);
	
	// QImageConsumer methods
	void end();
	void changed(const QRect&);
	void frameDone();
	void frameDone(const QPoint& offset, const QRect& rect);
	void setLooping(int);
	void setFramePeriod(int milliseconds);
	void setSize(int, int);

private slots:
	void start();
	void startLoading();
	void slotDataReceived(KIO::Job*, const QByteArray& chunk);
	void slotResult(KIO::Job*);
	void slotStatResult(KIO::Job*);
	void decodeChunk();
	void slotImageDecoded();
	void slotDecoderThreadFailed();
};

#endif /* GVDOCUMENTDECODEIMPL_H */
