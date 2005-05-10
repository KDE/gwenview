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

#ifndef GVSLIDESHOW_H
#define GVSLIDESHOW_H

// Qt
#include <qobject.h>

// KDE
#include <kurl.h>
#include "libgwenview_export.h"
class QTimer;

class GVDocument;
class KConfig;
class GVImageLoader;

class LIBGWENVIEW_EXPORT GVSlideShow : public QObject
{
Q_OBJECT
public:
	GVSlideShow(GVDocument* document);
	virtual ~GVSlideShow();

	void setLoop(bool);
	bool loop() const { return mLoop; }
	
	void setDelay(int);
	int delay() const { return mDelay; }
	
	void start(const KURL::List& urls);
	void stop();

	void readConfig(KConfig* config,const QString& group);
	void writeConfig(KConfig* config,const QString& group) const;

signals:
	void nextURL( const KURL& );
	void finished();

private slots:
	void slotTimeout();
	void slotLoaded();
	void prefetchDone( bool ok );

private:
	void prefetch();
	QTimer* mTimer;
	int mDelay;
	bool mLoop;
	GVDocument* mDocument;
	bool mStarted;
	KURL::List mURLs;
	KURL::List::ConstIterator mStartIt;
	GVImageLoader* mPrefetch;
	int mPrefetchAdvance;
};

#endif // GVSLIDESHOW_H
