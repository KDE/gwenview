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
#ifndef GVIMAGELOADER_H
#define GVIMAGELOADER_H

// Qt
#include <qasyncimageio.h>
#include <qbuffer.h>
#include <qcstring.h>

// KDE
#include <kio/job.h>

// Local 
#include "tsthread/tsthread.h"
#include "gvimageframe.h"
#include "gvbusylevelmanager.h"
#include "libgwenview_export.h"
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

class GVImageLoaderPrivate;

class LIBGWENVIEW_EXPORT GVImageLoader : public QObject, public QImageConsumer {
Q_OBJECT
public:
	static GVImageLoader* loader( const KURL& url ); // use this instead of ctor
	void release(); // use this instead of dtor (disconnect from signals before)

	QImage processedImage() const;
	GVImageFrames frames() const;
	QCString imageFormat() const;
	QByteArray rawData() const;
	KURL url() const;
	QSize knownSize() const;
	QRegion loadedRegion() const; // valid parts of processedImage()
	bool completed() const;

signals:
	void sizeLoaded(int, int);
	void imageChanged(const QRect&); // use processedImage(), is not in frames() yet
	void frameLoaded();
	void imageLoaded( bool ok );

private slots:
	void slotStatResult(KIO::Job*);
	void slotDataReceived(KIO::Job*, const QByteArray& chunk);
	void slotGetResult(KIO::Job*);
	void decodeChunk();
	void slotImageDecoded();
	void slotDecoderThreadFailed();
	void slotBusyLevelChanged( GVBusyLevel );

private:
	GVImageLoader();
	~GVImageLoader();
	void ref();
	void deref();
	void startLoading( const KURL& url );
	void suspendLoading();
	void resumeLoading();
	void finish( bool ok );
	void startThread();
	
	// QImageConsumer methods
	void end();
	void changed(const QRect&);
	void frameDone();
	void frameDone(const QPoint& offset, const QRect& rect);
	void setLooping(int);
	void setFramePeriod(int milliseconds);
	void setSize(int, int);

	GVImageLoaderPrivate* d;
};

#endif /* GVIMAGELOADER_H */
