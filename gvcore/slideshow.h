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

	/**
	 * never end automatically
	 * @param loop true to run in a loop
	 */
	void setLoop(bool loop);
	/** @return current loop status */
	bool loop() const { return mLoop; }
	
	void setDelay(int);
	/** return current delay value */
	int delay() const { return mDelay; }
	/** @return current delay value, milli seconds->value*1000, seconds->value */
	int delayTimer() const;

	/**
	 * suffix of the delay spin box
	 * @param suffix the suffix
	 */
	void setDelaySuffix(QString suffix);
	/** @return delay spin box suffix as QString */
	QString delaySuffix() const { return mDelaySuffix; }

	/**
	 * show fullscreen
	 * @param fullscreen true to show the slideshow in fullscreen
	 */
	void setFullscreen(bool fullscreen);
	/** @return return current fullscreen mode */
	bool fullscreen() const { return mFullscreen; }

	/**
	 * stop at the directory end
	 * @param stop true to stop
	 */
	void setStopAtEnd(bool stop) { mStopAtEnd=stop; }
	/** @return current stop value */
	bool stopAtEnd() const { return mStopAtEnd; }

	/**
	 * show images in a random order
	 * @param random true to show in random order
	 */
	void setRandom(bool random);
	/** @return current random status */
	bool random() const { return mRandom; }
	
	void start(const KURL::List& urls);
	void stop();

	/** @return true if the slideshow is running */
	bool isRunning() { return mStarted; }
	
	void readConfig(KConfig* config,const QString& group);
	void writeConfig(KConfig* config,const QString& group) const;

signals:
	void nextURL( const KURL& );
	void finished();
	void playImagesFinished();

private slots:
	void slotTimeout();
	void slotLoaded();
	void prefetchDone();

private:
	void prefetch();
	QTimer* mTimer;
	/** @brief random mode */
	bool mRandom;
	/** @brief if the file list is at the end it is automatically stopped if true */
	bool mStopAtEnd;
	/** @brief delay value between the loaded image and the next image in (milli) seconds */
	int mDelay;
	/** @brief suffix milliseconds or seconds */
	QString mDelaySuffix;
	/** @brief if the current image is one before the start image the slideshow will stop if true */
	bool mLoop;
	/** @brief show fullscreen if true */
	bool mFullscreen;
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

