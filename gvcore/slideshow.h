// vim: set tabstop=4 shiftwidth=4 noexpandtab
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau
 
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

#ifndef SLIDESHOW_H
#define SLIDESHOW_H

// Qt
#include <qobject.h>
#include <qvaluevector.h>

// KDE
#include <kurl.h>
#include "libgwenview_export.h"
class QTimer;

class KConfig;

namespace Gwenview {
class Document;
class ImageLoader;

class LIBGWENVIEW_EXPORT SlideShow : public QObject
{
Q_OBJECT
public:
	SlideShow(Document* document);
	virtual ~SlideShow();
	
	void start(const KURL::List& urls);
	void stop();

	/** @return true if the slideshow is running */
	bool isRunning() { return mStarted; }

public slots:
	void slotSettingsChanged();

signals:
	void nextURL( const KURL& );
	/**
	 * Slideshow has been started or stopped
	 */
	void stateChanged(bool running);

private slots:
	void slotTimeout();
	void slotLoaded();
	void prefetchDone();

private:
	QValueVector<KURL>::ConstIterator findNextURL() const;
	void prefetch();

	QTimer* mTimer;
	Document* mDocument;
	bool mStarted;
	QValueVector<KURL> mURLs;
	QValueVector<KURL>::ConstIterator mStartIt;
	ImageLoader* mPrefetch;
	int mPrefetchAdvance;
	KURL mPriorityURL;
};

} // namespace
#endif // SLIDESHOW_H

