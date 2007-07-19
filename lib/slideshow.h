// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef SLIDESHOW_H
#define SLIDESHOW_H

#include "gwenviewlib_export.h"

// Qt
#include <QObject>
#include <QVector>

// KDE
#include <kurl.h>
class QTimer;

namespace Gwenview {

class GWENVIEWLIB_EXPORT SlideShow : public QObject {
	Q_OBJECT
public:
	SlideShow(QObject* parent);
	virtual ~SlideShow();

	void start(const QList<KUrl>& urls);
	void stop();

	/** @return true if the slideshow is running */
	bool isRunning() { return mStarted; }

	void setCurrentUrl(const KUrl& url);

Q_SIGNALS:
	void goToUrl( const KUrl& );
	/**
	 * Slideshow has been started or stopped
	 */
	void stateChanged(bool running);

private Q_SLOTS:
	void slotTimeout();

private:
	QVector<KUrl>::ConstIterator findNextUrl() const;
	int timerInterval();

	QTimer* mTimer;
	bool mStarted;
	QVector<KUrl> mUrls;
	QVector<KUrl>::ConstIterator mStartIt;
	KUrl mCurrentUrl;
};

} // namespace

#endif // SLIDESHOW_H
